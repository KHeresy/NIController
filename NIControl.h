#pragma once
#pragma region Header Files
// Qt Header
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
	float	m_fJointConfidence;

public:
	QNIControl( bool bFrameless = true );
	~QNIControl();

	bool InitialNIDevice( int w = 640, int h = 480 );

	void Start()
	{
		startTimer( 25 );
	}

	void SetFramless( bool bTrue );

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

	void timerEvent( QTimerEvent* pEvent );

private:
	enum EControlHand
	{
		NICH_NO_HAND,
		NICH_RIGHT_HAND,
		NICH_LEFT_HAND,
	};

private:
	bool			m_bFrameless;
	EControlHand	m_eControlHand;

	QGraphicsScene	m_qScene;
	QGraphicsView	m_qView;
	QGridLayout		m_qLayout;

	QONI_UserMap*	m_pUserMap;
	QHandControl	m_mHandControl;

	openni::Device		m_niDevice;
	openni::VideoStream	m_niDepthStream;
	nite::UserTracker	m_niUserTracker;
};
