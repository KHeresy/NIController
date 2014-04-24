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
	enum EControlHand
	{
		NICH_NO_HAND,
		NICH_RIGHT_HAND,
		NICH_LEFT_HAND,
	};

	enum EControlStatus
	{
		NICS_STANDBY,
		NICS_WAIT_FIX,
		NICS_FIXED,
		NICS_INPUT,
	};

public:
	QTranWidget( nite::UserTracker& rUT, bool bFrameless = true ) :
		QWidget(), m_rUserTracker( rUT ), m_qScene(), m_qView( &m_qScene, this ), m_qLayout(this)
	{
		m_eControlStatus	= NICS_STANDBY;
		m_eControlHand		= NICH_NO_HAND;

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
		QTranWidget* pThis = this;
		m_pHand->m_mFunc = [pThis](){
			pThis->m_eControlStatus = QTranWidget::NICS_INPUT;
		};
		m_qScene.addItem( m_pHand );
		m_pHand->setZValue( 1 );
		m_pHand->hide();

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
		float fJointConTh = 0.5f;
		float fFixZTh = -300;
		float fMoveTh = 30;
		boost::chrono::milliseconds tdFixTime(100);

		if( m_pUserMap->Update() )
		{
			EControlHand	eHandStatus = NICH_NO_HAND;
			#pragma region select nearest hand
			float	fRC = m_pUserMap->GetActiveUserJoint( nite::JOINT_RIGHT_HAND ).getPositionConfidence(),
					fLC = m_pUserMap->GetActiveUserJoint( nite::JOINT_LEFT_HAND ).getPositionConfidence();

			if( fRC > fJointConTh )
			{
				if( fLC > fJointConTh )
				{
					QVector3D	posR = m_pUserMap->GetActiveUserJointTR( nite::JOINT_RIGHT_HAND ),
								posL = m_pUserMap->GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
					if( posR.z() > posL.z() )
						eHandStatus = NICH_LEFT_HAND;
					else
						eHandStatus = NICH_RIGHT_HAND;
				}
				else
				{
					eHandStatus = NICH_RIGHT_HAND;
				}
			}
			else if( fLC > fJointConTh )
			{
				eHandStatus = NICH_LEFT_HAND;
			}
			#pragma endregion

			if( eHandStatus == NICH_NO_HAND )
			{
				m_eControlHand = NICH_NO_HAND;
				m_eControlStatus = NICS_STANDBY;
				m_pHand->hide();
			}
			else
			{
				// get hand info
				QVector3D	mHandPos3D;
				QPointF		mHandPos2D;
				if( eHandStatus == NICH_RIGHT_HAND )
				{
					mHandPos3D = m_pUserMap->GetActiveUserJointTR( nite::JOINT_RIGHT_HAND );
					mHandPos2D = m_pUserMap->GetActiveUserJoint2D( nite::JOINT_RIGHT_HAND );
				}
				else
				{
					mHandPos3D = m_pUserMap->GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
					mHandPos2D = m_pUserMap->GetActiveUserJoint2D( nite::JOINT_LEFT_HAND );
				}

				// hand changed
				if( eHandStatus != m_eControlHand )
				{
					m_eControlHand = eHandStatus;
					m_eControlStatus = NICS_STANDBY;
					m_aTrackList.clear();
				}

				// add current position into track list
				auto tpNow = boost::chrono::system_clock::now();
				m_aTrackList.push_back( std::make_pair( tpNow, mHandPos3D ) );

				if( m_eControlStatus == NICS_STANDBY )
					m_eControlStatus = NICS_WAIT_FIX;

				if( m_eControlStatus == NICS_WAIT_FIX )
				{
					if( mHandPos3D.z() < fFixZTh )
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
							m_eControlStatus = NICS_FIXED;
	
							// update hand icon position
							m_pHand->resetTransform();
							m_pHand->translate( mHandPos2D.x() + 25, mHandPos2D.y() + 25 );	//TODO: Should not shift here (magic number?)
							m_pHand->show();
						}
					}
				}

				// check if hand move out from button
				if( m_eControlStatus == NICS_FIXED )
				{
					if( !m_pHand->CheckHand( mHandPos2D.x(), mHandPos2D.y() ) )
					{
						m_eControlStatus = NICS_WAIT_FIX;
						m_pHand->hide();
					}
				}

				if( m_eControlStatus == NICS_INPUT )
				{
					//TODO: the control after fix
				}
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
	EControlStatus	m_eControlStatus;
	EControlHand	m_eControlHand;

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
