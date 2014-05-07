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

#include "NIButton.h"
#pragma endregion

/**
 * The icon of hand position
 */
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
	QPen	m_penGeneral;			/**< The pen used in HS_GENERAL status */
	QPen	m_penFixing;			/**< The pen used in HS_FIXING status */
	QPen	m_penFixingProgress;	/**< The pen used in HS_FIXING status for progress */
	QPen	m_penFixed;				/**< The pen used in HS_FIXED status (border) */
	QBrush	m_brushFixed;			/**< The brush used in HS_FIXED status (fill) */

public:
	QHandIcon( float fSize = 50 )
	{
		m_eStatus	= HS_GENERAL;
		m_fProgress	= 0.0f;

		// drawing parameter in HS_GENERAL status
		m_penGeneral.setColor( qRgba( 255, 255, 0, 128 ) );
		m_penGeneral.setWidth( 5 );

		// drawing parameter in HS_FIXING status
		m_penFixing.setColor( qRgba( 255, 255, 0, 255 ) );
		m_penFixing.setWidth( 5 );

		m_penFixingProgress.setColor( qRgba( 0, 255, 0, 255 ) );
		m_penFixingProgress.setWidth( 15 );

		// drawing parameter in HS_FIXED status
		m_penFixed.setColor( qRgba( 0, 0, 0, 0 ) );
		m_brushFixed = QBrush(qRgba( 255, 0, 0, 128 ));

		SetSize( fSize );
		hide();
	}

	/**
	 * Set the size of the hand icon
	 */
	void SetSize( float fSize )
	{
		float fHS = fSize / 2;
		m_Rect.setRect( -fHS, -fHS, fSize, fSize );
	}

	/**
	 * Update the status of the hand (the draw method will be changed)
	 */
	void SetStatus( const HAND_STATE& eStatus )
	{
		m_eStatus = eStatus;
	}

	/**
	 * Update the hand icon progress. Only make effect when status is HS_FIXING
	 */
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

/**
 * Hand control system
 */
class QHandControl : public QGraphicsItemGroup
{
public:
	float							m_fHandMoveThreshold;	/**< The movement threshold for fixing hand (2D) */
	float							m_fHandForwardDistance;	/**< The forward distance threshold for initial fix hand */
	boost::chrono::milliseconds		m_tdPreFixTime;			/**< The time start to fix */
	boost::chrono::milliseconds		m_tdFixTime;			/**< The time to fix */
	boost::chrono::milliseconds		m_tdInvokeTime;			/**< The time to invoke button */	//TODO: no work now
	std::function<void()>			m_funcStartInput;
	std::function<void()>			m_funcEndInput;

public:
	QHandControl()
	{
		m_fHandMoveThreshold	= 25;
		m_fHandForwardDistance	= 250;
		m_tdPreFixTime			= boost::chrono::milliseconds( 100 );
		m_tdFixTime				= boost::chrono::milliseconds( 500 );
		m_tdInvokeTime			= boost::chrono::milliseconds( 300 );
		m_funcStartInput		= [](){};
		m_funcEndInput			= [](){};

		m_aTrackList.set_capacity( 150 );
		SetRect( QRectF( 0, 0, 640, 480 ) );
		BuildButtons();

		addToGroup( &m_HandIcon );
		addToGroup( &m_qButtons );

		m_eControlStatus	= NICS_INPUT;
		UpdateStatus( NICS_NO_HAND );
	}

	/**
	 * Reset hand status, clear history
	 */
	void HandReset()
	{
		UpdateStatus( NICS_STANDBY );
		m_aTrackList.clear();
	}

	/**
	 * Update current hand point information
	 */
	void UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D );

	/**
	 * Set the hand as lost
	 */
	void HandLost()
	{
		UpdateStatus( NICS_NO_HAND );
	}

	/**
	 * Set the region of this widget
	 */
	void SetRect( const QRectF& rRect )
	{
		m_qRect = rRect;

		m_HandIcon.SetSize( rRect.width() / 12 );
	}

	QRectF boundingRect() const
	{
		return m_qRect;
	}

private:
	typedef	boost::chrono::system_clock::time_point TTimePoint;

	enum EControlStatus
	{
		NICS_NO_HAND,
		NICS_STANDBY,
		NICS_FIXING,
		NICS_FIXED,
		NICS_INPUT,
	};

	/**
	 * Structure to save required data at each yime point
	 */
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

	template<typename _TD1, typename _TD2>
	float ComputeProgress( const _TD1& time1, const _TD2& time2 )
	{
		return float(boost::chrono::duration_cast<_TD2>( time1 ).count()) / time2.count();
	}

private:
	QHandIcon			m_HandIcon;
	QGraphicsItemGroup	m_qButtons;
	QRectF				m_qRect;
	EControlStatus		m_eControlStatus;

	SHandPos	m_FixPos;
	boost::circular_buffer<SHandPos>	m_aTrackList;
	std::vector<QBaseProgressButton*>	m_vButtons;
	std::vector<QBaseProgressButton*>::iterator	m_itCurrentButton;
	TTimePoint		m_tpFirstIn;
};
