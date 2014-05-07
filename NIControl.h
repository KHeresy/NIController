#pragma once
#pragma region Header Files
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

// NIController window
class QNIControl : public QWidget
{
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
		int w = 640, h = 480;
		if( a.length() == 2 )
		{
			w = a[0].toInt();
			h = a[1].toInt();
		}
		return InitialNIDevice( w, h );
	}
	bool InitialNIDevice( int w, int h );

	/**
	 * Start to read data
	 */
	void Start()
	{
		startTimer( 25 );
	}

	/**
	 * Set window as framless (transparence)
	 */
	void SetFramless( bool bTrue );

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
	QRectF			m_qRect;			/**< define the visible range, default 640*480 */

	QSettings		m_qSetting;			/**< INI configuration */
	float			m_fJointConfidence;	/**< The confidence value of joint position to use */

	bool			m_bFrameless;		/**< Internal flag: if this window is frameless */
	EControlHand	m_eControlHand;		/**< Internal flag: which hand is used to control now */

	QGraphicsScene	m_qScene;
	QGraphicsView	m_qView;
	QGridLayout		m_qLayout;

	QONI_UserMap	m_mUserMap;			/**< handle the data update of NiTE userTracker, and the drawing of depth map, user map, user skeleton, user direction */
	QHandControl	m_mHandControl;		/**< handle the hand position analyze, hand icon and buttons update */

	openni::Device		m_niDevice;
	openni::VideoStream	m_niDepthStream;
	nite::UserTracker	m_niUserTracker;
};
