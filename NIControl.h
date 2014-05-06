#pragma once
#pragma region Header Files
// STL Header
#include <array>

// Qt Header
#include <QtCore/QSettings>
#include <QtGui/QtGui>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

// Application header
#include "UserMap.h"
#include "HandControl.h"
#pragma endregion

// Main Window
class QNIControl : public QWidget
{
public:
	float		m_fJointConfidence;
	QSettings	m_qSetting;

public:
	QNIControl( QString sINIFile = "" );
	~QNIControl();

	/**
	 * Initial OpenNI and NiTE
	 */
	bool InitialNIDevice()
	{
		QString s = m_qSetting.value( "OpenNI/Resolution", "640/480" ).toString();
		QStringList a = s.split('/');
		if( a.length() == 2 )
		{
			m_aResoultion[0] = a[0].toInt();
			m_aResoultion[1] = a[1].toInt();
		}
		else
		{
			m_aResoultion[0] = 640;
			m_aResoultion[1] = 480;
		}
		return InitialNIDevice( m_aResoultion[0], m_aResoultion[1] );
	}

	bool InitialNIDevice( int w, int h );

	void Start()
	{
		startTimer( 25 );
	}

	void SetFramless( bool bTrue );

	void SetSkeletonSmoothing( float fValue )
	{
		m_niUserTracker.setSkeletonSmoothingFactor( fValue );
	}

private:
	bool eventFilter(QObject *object, QEvent *event)
	{
		// block page down and up in self
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
		// handle mode switch
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
		m_qView.fitInView( &m_mUserMap, Qt::KeepAspectRatio );
	}

	void timerEvent( QTimerEvent* pEvent );

private:
	enum EControlHand
	{
		NICH_NO_HAND,
		NICH_RIGHT_HAND,
		NICH_LEFT_HAND,
	};

private:
	std::array<unsigned int,2>	m_aResoultion;
	bool			m_bFrameless;
	EControlHand	m_eControlHand;

	QGraphicsScene	m_qScene;
	QGraphicsView	m_qView;
	QGridLayout		m_qLayout;

	QONI_UserMap	m_mUserMap;
	QHandControl	m_mHandControl;

	openni::Device		m_niDevice;
	openni::VideoStream	m_niDepthStream;
	nite::UserTracker	m_niUserTracker;
};
