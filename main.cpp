#pragma region Header Files

// STL Header
#include <array>
#include <iostream>
#include <sstream>

// Eigen
#include <Eigen/Dense>
#include <Eigen/Geometry>

// Qt Header
#include <QtGui/QtGui>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

// Application header
#include "NIButton.h"
#include "UserMap.h"

// windows header
#include <Windows.h>
#pragma endregion

float g_fFaceCond = 0.8;

void SendKey( WORD key )
{
	INPUT mWinEvent;
	mWinEvent.type = INPUT_KEYBOARD;
	mWinEvent.ki.time = 0;
	mWinEvent.ki.dwFlags = 0;
	mWinEvent.ki.wScan = 0;
	mWinEvent.ki.wVk = key;
	SendInput( 1, &mWinEvent, sizeof(mWinEvent) );
}

class QDirection : public QGraphicsItem
{
public:
	QDirection( int uSize ) : QGraphicsItem()
	{
		m_qRect = QRectF( QPointF( 0, 0 ), QSizeF(uSize,uSize) );
		m_vDir = QVector2D( 0, -1 );
	}

	QRectF boundingRect() const
	{
		return m_qRect;
	}

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		// p1
		QPen pen( qRgb( 255, 255, 0 ) );
		pen.setWidth( 3 );
		painter->setPen( pen );
		painter->drawEllipse( m_qRect );

		// p2
		pen.setColor( qRgb( 255, 0, 0 ) );
		painter->setPen( pen );
		pen.setWidth( 5 );
		float r = m_qRect.width() / 2;
		painter->drawLine( r, r, r + r * m_vDir.x(), r + r * m_vDir.y() );
	}

public:
	QRectF		m_qRect;
	QVector2D	m_vDir;
};

// Main Window
class QTranWidget : public QWidget
{
public:
	QTranWidget( nite::UserTracker& rUT, bool bFrameless = true ) :
		QWidget(), m_rUserTracker( rUT ), m_qScene(), m_qView( &m_qScene, this ), m_qLayout(this)
	{
		// configurate window
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAttribute(Qt::WA_TranslucentBackground, true);
		setAttribute(Qt::WA_PaintOnScreen);

		// sub-widget
		m_qLayout.setSpacing(0);
		m_qLayout.setMargin(0);
		m_qLayout.addWidget( &m_qView, 0, 0 );

		m_qView.setStyleSheet( "background: transparent; border: 0px;" );
		//m_qView.setBackgroundBrush(QBrush(qRgba( 255, 255, 255, 0 ), Qt::SolidPattern));
		m_qView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_qView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_qView.installEventFilter( this );

		// Scene
		m_pUserMap = new QONI_UserMap( m_rUserTracker );
		m_qScene.addItem( m_pUserMap );

		m_pButtons		= new QGraphicsItemGroup();
		m_qScene.addItem( m_pButtons );

		m_pDirection = new QDirection( 40 );
		m_qScene.addItem( m_pDirection );
		m_pDirection->translate( 590, 430 );

		// buttons
		for( int i = 0; i < m_aButtons.size(); ++ i )
		{
			m_aButtons[i] = new QAbsNIButton( 200 );
			m_pButtons->addToGroup( m_aButtons[i] );
		}
		m_aButtons[0]->translate( -100, 0 );
		m_aButtons[0]->m_mFunc = [](){ SendKey(VK_PRIOR); };
		m_aButtons[1]->translate( 100, 0 );
		m_aButtons[1]->m_mFunc = [](){ SendKey(VK_NEXT); };
		m_pButtons->translate( 320, 200 );

		m_pHandIcon			= m_qScene.addEllipse( QRectF( -10, -10, 10, 10 ), QPen( qRgba(0,0,0,0) ), QBrush( qRgba(255,128,128,128) ) );
		m_pHandIcon->hide();

		SetFramless( bFrameless );
	}

	void Start()
	{
		startTimer( 25 );
	}

	void SetFramless( bool bTrue )
	{
		m_bFrameless = bTrue;
		auto mPos = pos();
		if( m_bFrameless )
		{
			setWindowOpacity(0.5);
			setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
		}
		else
		{
			setWindowOpacity(1.0);
			setWindowFlags( Qt::WindowStaysOnTopHint );
		}
		this->show();
		this->move( mPos );
	}

private:
	bool eventFilter(QObject *object, QEvent *event)
	{
		if( event->type() == QEvent::KeyPress )
		{
			QKeyEvent *ke = static_cast<QKeyEvent *>(event);
			if( ke->key() == Qt::Key_PageDown || ke->key() == Qt::Key_PageUp )
			{
				return true;
			}
		}
		return false;
	}

	void keyPressEvent( QKeyEvent* pEvent )
	{
		switch( pEvent->key() )
		{
		case Qt::Key_F:
			if( !isFullScreen() )
				setWindowState(Qt::WindowFullScreen);
			else
				setWindowState( windowState() ^ Qt::WindowFullScreen );
			break;

		case Qt::Key_M:
			SetFramless( !m_bFrameless );
			break;
		}
	}

	void resizeEvent( QResizeEvent* pEvent )
	{
		m_qView.fitInView( m_pUserMap, Qt::KeepAspectRatio );
	}

	void timerEvent( QTimerEvent* pEvent )
	{
		float fCon = 0.5f;
		bool bCanControl = false;

		if( m_pUserMap->Update() )
		{
			// check troso data
			const auto& rTorso = m_pUserMap->GetActiveUserJoint( nite::JOINT_TORSO );
			if( rTorso.getPositionConfidence() > fCon && rTorso.getOrientationConfidence() > fCon )
			{
				// compute the face direction
				auto& rQ = rTorso.getOrientation();
				QVector3D vZD( 0, 0, -1 );
				auto vZ = QQuaternion( rQ.w, rQ.x, rQ.y, rQ.z ).rotatedVector( vZD );
				
				// ignore Y axis
				QVector2D v2DDir = QVector2D( vZ.x(), vZ.z() ).normalized();
				m_pDirection->m_vDir.setX( v2DDir.x() );
				m_pDirection->m_vDir.setY( v2DDir.y() );

				// check if face the sensor
				if( v2DDir.y() < -g_fFaceCond )
				{
					const auto& rRHand = m_pUserMap->GetActiveUserJoint( nite::JOINT_RIGHT_HAND );
					const auto& rLHand = m_pUserMap->GetActiveUserJoint( nite::JOINT_LEFT_HAND );

					// check use which hand
					QPointF			mHandPos2D;
					nite::Point3f	mHandPos3D;

					auto funcUseRightHand = [&mHandPos2D,&mHandPos3D](QONI_UserMap& rUMP){
						mHandPos3D = rUMP.GetActiveUserJoint(nite::JOINT_RIGHT_HAND).getPosition();
						mHandPos2D = rUMP.GetActiveUserJoint2D(nite::JOINT_RIGHT_HAND);
					};

					auto funcUseLeftHand = [&mHandPos2D,&mHandPos3D](QONI_UserMap& rUMP){
						mHandPos3D = rUMP.GetActiveUserJoint(nite::JOINT_LEFT_HAND).getPosition();
						mHandPos2D = rUMP.GetActiveUserJoint2D(nite::JOINT_LEFT_HAND);
					};

					// check two hands
					bCanControl = true;
					if( rRHand.getPositionConfidence() > fCon && rLHand.getPositionConfidence() < fCon )
					{
						funcUseRightHand( *m_pUserMap );
					}
					else if( rRHand.getPositionConfidence() < fCon && rLHand.getPositionConfidence() > fCon )
					{
						funcUseLeftHand( *m_pUserMap );
					}
					else if( rRHand.getPositionConfidence() > fCon && rLHand.getPositionConfidence() > fCon )
					{
						if( rRHand.getPosition().z > rLHand.getPosition().z )
							funcUseLeftHand( *m_pUserMap );
						else
							funcUseRightHand( *m_pUserMap );
					}
					else
					{
						bCanControl = false;
					}
					
					if( bCanControl )
					{
						// show buttons
						auto rTPos = rTorso.getPosition();
						if( !m_pButtons->isVisible() )
						{
							float tx, ty;
							m_rUserTracker.convertJointCoordinatesToDepth( rTPos.x, rTPos.y, rTPos.z, &tx, &ty );
							m_pButtons->resetTransform();
							m_pButtons->translate( tx, ty );
							m_pButtons->show();
						}

						// compute the color of icon
						float fZTh = 250;
						if( mHandPos3D.z - rTPos.z < -fZTh )
							m_pHandIcon->setBrush( QBrush( qRgba(255,0,0,128) ) );
						else
							m_pHandIcon->setBrush( QBrush( qRgba(255,128,128,128) ) );

						// check each button
						for( auto itB = m_aButtons.begin(); itB != m_aButtons.end(); ++ itB )
						{
							if( (*itB)->CheckHand( mHandPos2D.x(), mHandPos2D.y(), mHandPos3D.z ) )
							{
							}
						}

						// update hand icon position
						m_pHandIcon->resetTransform();
						m_pHandIcon->translate( mHandPos2D.x(), mHandPos2D.y() );

						// show hand icon
						m_pHandIcon->show();
					}
				}
			}
		}
		if( !bCanControl )
		{
			m_pHandIcon->hide();
			m_pButtons->hide();
		}

		m_qView.fitInView( 0, 0, 640, 480, Qt::KeepAspectRatio  );
	}

	void mousePressEvent( QMouseEvent *event )
	{
		m_qMouseShift = event->globalPos() - pos();
		std::cout << "Pressed" << std::endl;
	}

	void mouseMoveEvent( QMouseEvent *event )
	{
		std::cout << "Moved" << std::endl;
		move(event->globalPos() - m_qMouseShift);
	}

private:
	bool	m_bFrameless;

	QPoint			m_qMouseShift;
	QGraphicsScene	m_qScene;
	QGraphicsView	m_qView;
	QGridLayout		m_qLayout;
	QONI_UserMap*			m_pUserMap;
	QGraphicsEllipseItem*	m_pHandIcon;
	QGraphicsItemGroup*		m_pButtons;
	QDirection*				m_pDirection;
	std::array<QAbsNIButton*,2>	m_aButtons;

	nite::UserTracker&		m_rUserTracker;
};

int main( int argc, char** argv )
{
	if( argc > 1 )
	{
		g_fFaceCond = atof( argv[1] );
	}

	bool bFrameless = false;

	#pragma region Qt Core
	// Qt Application
	QApplication qOpenNIApp( argc, argv );
	#pragma endregion

	#pragma region OpenNI
	if( openni::OpenNI::initialize() != openni::STATUS_OK )
	{
		QMessageBox::critical( NULL, "OpenNI initialize error", openni::OpenNI::getExtendedError() );
		return -1;
	}

	openni::Device devDevice;
	if( devDevice.open( openni::ANY_DEVICE ) != openni::STATUS_OK )
	{
		QMessageBox::critical( NULL, "Can't open OpenNI Device", openni::OpenNI::getExtendedError() );
		return -1;
	}

	openni::VideoStream vsDepth;
	if( vsDepth.create( devDevice, openni::SENSOR_DEPTH ) != openni::STATUS_OK )
	{
		QMessageBox::critical( NULL, "Can't create depth stream", openni::OpenNI::getExtendedError() );
		return -1;
	}
	else
	{
		openni::VideoMode mMode;
		mMode.setFps( 30 );
		mMode.setResolution( 640, 480 );
		mMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
		vsDepth.setVideoMode( mMode );
	}
	#pragma endregion

	#pragma region NiTE
	if( nite::NiTE::initialize() != nite::STATUS_OK )
	{
		QMessageBox::critical( NULL, "NiTE", "NiTE initialize error" );
		return -1;
	}

	nite::UserTracker mUserTracker;
	if( mUserTracker.create( &devDevice ) != nite::STATUS_OK )
	{
		QMessageBox::critical( NULL, "User Tracker", "UserTracker created failed" );
		return -1;
	}
	mUserTracker.setSkeletonSmoothingFactor( 0.25f );

	#pragma endregion
	
	#pragma region Qt Widget
	// Qt Window
	QTranWidget qWin(mUserTracker,bFrameless);
	qWin.show();
	qWin.resize( 640, 480 );
	#pragma endregion

	// main loop
	qWin.Start();
	return qOpenNIApp.exec();
}
