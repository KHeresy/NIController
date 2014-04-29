#pragma once

#pragma region Header Files
// STL Header
#include <functional>
#include <vector>

// Boost Header
#include <boost/chrono.hpp>
#include <boost/circular_buffer.hpp>

// QT Header
#include <QtGui/QtGui>

#pragma endregion

class QHandIcon : public QGraphicsItem
{
public:
	enum HAND_STATE
	{
		HS_GENERAL,
		HS_FIXING,
		HS_FIXED
	};

public:
	QHandIcon( float fSize = 50 )
	{
		m_eStatus	= HS_GENERAL;
		m_fProgress	= 0.0f;

		SetSize( fSize );
		hide();
	}

	void SetSize( float fSize )
	{
		float fHS = fSize / 2;
		m_Rect.setRect( -fHS, -fHS, fSize, fSize );
	}

	void SetStatus( const HAND_STATE& eStatus )
	{
		m_eStatus = eStatus;
	}

	void SetProgress( const float& fVal )
	{
		m_fProgress = fVal;
	}

	QRectF boundingRect() const
	{
		return m_Rect;
	}

	void paint( QPainter *pPainter, const QStyleOptionGraphicsItem *option, QWidget *widget );

private:
	QRectF		m_Rect;
	HAND_STATE	m_eStatus;
	float		m_fProgress;
};

class QHandControl : public QGraphicsItemGroup
{
public:
	typedef	boost::chrono::system_clock::time_point TTimePoint;

	enum EControlStatus
	{
		NICS_NO_HAND,
		NICS_STANDBY,
		NICS_FIXING,
		NICS_FIXED,
		NICS_INPUT,
	};

	struct SHandPos
	{
		TTimePoint	tpTime;
		QPointF		mPos2D;
		QVector3D	mPos3D;

		SHandPos(){}

		SHandPos( const QPointF& pos2D, const QVector3D& pos3D )
		{
			tpTime = boost::chrono::system_clock::now();
			mPos2D = pos2D;
			mPos3D = pos3D;
		}
	};

public:
	float	m_fHandIconSize;
	float	m_fHandMoveThreshold;
	float	m_fHandForwardDistance;
	boost::chrono::milliseconds	m_tdPreFixTime;
	boost::chrono::milliseconds	m_tdFixTime;

public:
	QHandControl()
	{
		m_fHandIconSize			= 50;
		m_fHandMoveThreshold	= 25;
		m_fHandForwardDistance	= 300;
		m_tdPreFixTime			= boost::chrono::milliseconds( 200 );
		m_tdFixTime				= boost::chrono::milliseconds( 1000 );

		m_HandIcon.SetSize( m_fHandIconSize );

		m_aTrackList.set_capacity( 150 );
		m_qRect.setRect( 0, 0, 640, 480 );
		BuildButtons();

		addToGroup( &m_HandIcon );
		addToGroup( &m_qButtons );

		m_eControlStatus	= NICS_INPUT;
		UpdateStatus( NICS_NO_HAND );
	}

	void HandReset()
	{
		UpdateStatus( NICS_STANDBY );
		m_aTrackList.clear();
	}

	void UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D );

	void HandLost()
	{
		UpdateStatus( NICS_NO_HAND );
	}

	void SetRect( const QRectF& rRect )
	{
		m_qRect = rRect;
	}

	QRectF boundingRect() const
	{
		return m_qRect;
	}

private:
	template<typename _TDuration>
	bool Is2DPosFixFor( const _TDuration& rDuration, float fMoveRange = 10 )
	{
		if( m_aTrackList.size() < 2 )
			return false;

		auto itStartPt = m_aTrackList.rbegin();
		auto itPt = itStartPt + 1;
		for( ; itPt != m_aTrackList.rend(); ++ itPt )
		{
			// check position
			if( QLineF( itStartPt->mPos2D, itPt->mPos2D ).length() > fMoveRange )
				return false;

			// check time
			if( itStartPt->tpTime - itPt->tpTime > rDuration )
				return true;
		}
		return false;
	}

	bool UpdateStatus( const EControlStatus& eStatus );

	const SHandPos& CurrentPos() const
	{
		return m_aTrackList.back();
	}

	void BuildButtons();

private:
	QHandIcon			m_HandIcon;
	QGraphicsItemGroup	m_qButtons;
	QRectF				m_qRect;
	EControlStatus		m_eControlStatus;

	SHandPos	m_FixPos;
	boost::circular_buffer<SHandPos>	m_aTrackList;
	std::vector< std::pair<QGraphicsItem*,std::function<void()> > >	m_vButtons;
	QGraphicsItem*	m_pCurrentButton;
};
