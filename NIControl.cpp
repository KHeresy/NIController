#include "NIControl.h"

QNIControl::QNIControl( QString sINIFile ) :
	m_qSetting( sINIFile, QSettings::IniFormat ),
	QWidget(), m_qScene(), m_qView( &m_qScene, this ), m_qLayout(this), m_mUserMap( m_niUserTracker )
{
	m_qRect = QRectF( 0, 0, 640, 480 );

	m_fJointConfidence	= m_qSetting.value( "OpenNI/JointConfidence", 0.5f ).toFloat();
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
	m_qScene.addItem( &m_mUserMap );
	m_mUserMap.setZValue( 2 );
	//m_pUserMap->setOpacity( 0.5 );

	m_qScene.addItem( &m_mHandControl );
	m_mHandControl.setZValue( 1 );

	QONI_UserMap& rUMap = m_mUserMap;
	m_mHandControl.m_funcStartInput			= [&rUMap](){ rUMap.KeepSkeletonTransform( true ); };
	m_mHandControl.m_funcEndInput			= [&rUMap](){ rUMap.KeepSkeletonTransform( false ); };
	m_mHandControl.m_fHandMoveThreshold		= m_qSetting.value( "Control/MoveThreshold", 25 ).toFloat();
	m_mHandControl.m_fHandForwardDistance	= m_qSetting.value( "Control/ForwardDistance", 250 ).toFloat();
	m_mHandControl.m_tdPreFixTime			= boost::chrono::milliseconds( m_qSetting.value( "Control/PreFixTime", 100 ).toInt() );
	m_mHandControl.m_tdFixTime				= boost::chrono::milliseconds( m_qSetting.value( "Control/FixTime", 500 ).toInt() );
	m_mHandControl.m_tdInvokeTime			= boost::chrono::milliseconds( m_qSetting.value( "Control/InvokeTime", 200 ).toInt() );

	SetFramless( false );
}

QNIControl::~QNIControl()
{
	m_niUserTracker.destroy();
	nite::NiTE::shutdown();

	m_niDepthStream.destroy();
	m_niDevice.close();
	openni::OpenNI::shutdown();
}

bool QNIControl::InitialNIDevice( int w, int h )
{
	#pragma region OpenNI
	using namespace openni;
	if( OpenNI::initialize() != STATUS_OK )
	{
		QMessageBox::critical( NULL, "OpenNI initialize error", OpenNI::getExtendedError() );
		return false;
	}

	if( m_niDevice.open( ANY_DEVICE ) != STATUS_OK )
	{
		QMessageBox::critical( NULL, "Can't open OpenNI Device", OpenNI::getExtendedError() );
		return false;
	}

	if( m_niDepthStream.create( m_niDevice, SENSOR_DEPTH ) != STATUS_OK )
	{
		QMessageBox::critical( NULL, "Can't create depth stream", OpenNI::getExtendedError() );
		return false;
	}
	else
	{
		openni::VideoMode mMode;
		mMode.setFps( 30 );
		mMode.setResolution( w, h );
		mMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
		m_niDepthStream.setVideoMode( mMode );
	}
	#pragma endregion

	#pragma region NiTE
	using namespace nite;
	if( NiTE::initialize() != nite::STATUS_OK )
	{
		QMessageBox::critical( NULL, "NiTE", "NiTE initialize error" );
		return false;
	}

	if( m_niUserTracker.create( &m_niDevice ) != nite::STATUS_OK )
	{
		QMessageBox::critical( NULL, "User Tracker", "UserTracker created failed" );
		return false;
	}
	m_niUserTracker.setSkeletonSmoothingFactor( m_qSetting.value( "OpenNI/SkeletonSmooth", 0.75f ).toFloat() );
	#pragma endregion

	resize( m_qRect.width(), m_qRect.height() );
	m_mUserMap.SetSize( m_qRect.width(), m_qRect.height() );
	m_mHandControl.SetRect( m_qRect );

	return true;
}

void QNIControl::SetFramless( bool bTrue )
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

void QNIControl::timerEvent( QTimerEvent* pEvent )
{
	if( m_mUserMap.Update() )
	{
		EControlHand	eHandStatus = NICH_NO_HAND;
		#pragma region select nearest hand
		float	fRC = m_mUserMap.GetActiveUserJoint( nite::JOINT_RIGHT_HAND ).getPositionConfidence(),
				fLC = m_mUserMap.GetActiveUserJoint( nite::JOINT_LEFT_HAND ).getPositionConfidence();

		if( fRC > m_fJointConfidence )
		{
			if( fLC > m_fJointConfidence )
			{
				QVector3D	posR = m_mUserMap.GetActiveUserJointTR( nite::JOINT_RIGHT_HAND ),
							posL = m_mUserMap.GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
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
		else if( fLC > m_fJointConfidence )
		{
			eHandStatus = NICH_LEFT_HAND;
		}
		#pragma endregion

		if( eHandStatus == NICH_NO_HAND || eHandStatus != m_eControlHand )
		{
			m_mHandControl.HandLost();
			m_eControlHand = eHandStatus;
		}

		if( m_eControlHand != NICH_NO_HAND )
		{
			#pragma region General Hand position process
			// get hand info
			QVector3D	mHandPos3D;
			QPointF		mHandPos2D;
			if( eHandStatus == NICH_RIGHT_HAND )
			{
				mHandPos3D = m_mUserMap.GetActiveUserJointTR( nite::JOINT_RIGHT_HAND );
				mHandPos2D = m_mUserMap.GetActiveUserJoint2D( nite::JOINT_RIGHT_HAND );
			}
			else
			{
				mHandPos3D = m_mUserMap.GetActiveUserJointTR( nite::JOINT_LEFT_HAND );
				mHandPos2D = m_mUserMap.GetActiveUserJoint2D( nite::JOINT_LEFT_HAND );
			}

			// add current position into track list
			m_mHandControl.UpdateHandPoint( mHandPos2D, mHandPos3D );
			#pragma endregion
		}
	}

	m_qView.fitInView( m_qRect, Qt::KeepAspectRatio  );
}
