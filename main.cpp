#pragma region Header Files

// STL Header
#include <array>
#include <iostream>
#include <sstream>

// Boost Header
#include <boost/circular_buffer.hpp>

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

/**
 * Keyboard simulator
 */
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
		m_pUserMap->setZValue( 2 );

		m_pHand		= new QAbsNIButton( 100 );
		m_qScene.addItem( m_pHand );
		m_pHand->setZValue( 1 );

		m_aTrackList.set_capacity( 150 );

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
		float fZTh = -300;
		float fMoveTh = 30;
		boost::chrono::milliseconds tdFixTime(300);

		static bool bCanControl = false;

		if( m_pUserMap->Update() )
		{
			bool	bHasHand	= false,
					bRightHand	= true;
			QVector3D	mHandPos3D;
			QPointF		mHandPos2D;

			#pragma region select nearest hand
			float	fRC = m_pUserMap->GetActiveUserJoint( nite::JOINT_RIGHT_HAND ).getPositionConfidence(),
					fLC = m_pUserMap->GetActiveUserJoint( nite::JOINT_LEFT_HAND ).getPositionConfidence();

			if( fRC > fCon )
			{
				bHasHand = true;
				if( fLC > fCon )
				{
					QVector3D	posR = m_pUserMap->GetActiveUserJointTR( nite::JOINT_RIGHT_HAND ),
								posL = m_pUserMap->GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
					if( posR.z() > posL.z() )
						bRightHand = false;
				}
			}
			else if( fLC > fCon )
			{
				bHasHand = true;
				bRightHand = false;
			}
			#pragma endregion

			if( bHasHand )
			{
				// get hand info
				if( bRightHand )
				{
					mHandPos3D = m_pUserMap->GetActiveUserJointTR( nite::JOINT_RIGHT_HAND );
					mHandPos2D = m_pUserMap->GetActiveUserJoint2D( nite::JOINT_RIGHT_HAND );
				}
				else
				{
					mHandPos3D = m_pUserMap->GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
					mHandPos2D = m_pUserMap->GetActiveUserJoint2D( nite::JOINT_LEFT_HAND );
				}

				// add current position into track list
				auto tpNow = boost::chrono::system_clock::now();
				m_aTrackList.push_back( std::make_pair( tpNow, mHandPos3D ) );

				// check if hand is fix for 1 second
				if( bCanControl == false )
				{
					// check if hand position is fix long enough
					bool bFix = false;
					for( auto itPt = m_aTrackList.rbegin(); itPt != m_aTrackList.rend(); ++ itPt )
					{
						// check position
						if( ( mHandPos3D - itPt->second ).length() > fMoveTh )
							break;

						// check time
						if( tpNow - itPt->first > tdFixTime )
						{
							bFix = true;
							break;
						}
					}

					// start float hand button if fix
					if( bFix )
					{
						bCanControl = true;

						// update hand icon position
						m_pHand->resetTransform();
						m_pHand->translate( mHandPos2D.x() + 25, mHandPos2D.y() + 25 );	//TODO: Should not shift here
						m_pHand->show();
					}
				}
			}

			if( !bCanControl || !m_pHand->CheckHand( mHandPos2D.x(), mHandPos2D.y() ) )
			{
				bCanControl = false;
				m_pHand->hide();
			}
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
	QONI_UserMap*	m_pUserMap;
	QAbsNIButton*	m_pHand;

	nite::UserTracker&		m_rUserTracker;

	boost::circular_buffer< std::pair<boost::chrono::system_clock::time_point, QVector3D> >	m_aTrackList;
};

int main( int argc, char** argv )
{
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
	mUserTracker.setSkeletonSmoothingFactor( 0.5f );

	#pragma endregion
	
	#pragma region Qt Widget
	// Qt Window
	QTranWidget qWin(mUserTracker,false);
	qWin.show();
	qWin.resize( 640, 480 );
	#pragma endregion

	// main loop
	qWin.Start();
	return qOpenNIApp.exec();
}
