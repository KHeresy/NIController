#pragma once

// Boost Header
#include <boost/chrono.hpp>
#include <boost/circular_buffer.hpp>

// QT Header
#include <QtGui/qvector3d.h>

#include "NIButton.h"

class QHandControl : public QGraphicsItemGroup
{
public:
	typedef	boost::chrono::system_clock::time_point TTimePoint;

	enum EControlStatus
	{
		NICS_NO_HAND,	//TODO: not sure if this staus is required
		NICS_STANDBY,
		NICS_WAIT_FIX,
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
	QHandControl()
	{
		m_eControlStatus	= NICS_NO_HAND;
		m_aTrackList.set_capacity( 150 );
		m_qRect.setRect( 0, 0, 640, 480 );

		m_pHand		= new QAbsNIButton( 100 );
		QHandControl* pThis = this;
		m_pHand->m_mFunc = [pThis](){
			pThis->UpdateStatus( QHandControl::NICS_INPUT );
		};
		m_pHand->hide();
		addToGroup( m_pHand );
	}

	void HandReset()
	{
		m_eControlStatus	= NICS_STANDBY;
		m_aTrackList.clear();
		m_pHand->hide();
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

private:
	QAbsNIButton*	m_pHand;
	QRectF			m_qRect;
	EControlStatus	m_eControlStatus;

	SHandPos	m_FixPos;
	boost::circular_buffer<SHandPos>	m_aTrackList;
};

bool QHandControl::UpdateStatus( const QHandControl::EControlStatus& eStatus )
{
	if( m_eControlStatus != eStatus )
	{
		m_eControlStatus = eStatus;

		switch( m_eControlStatus )
		{
		case NICS_NO_HAND:
			HandReset();
			break;

		case NICS_FIXED:
			m_FixPos = CurrentPos();
			break;
		}
		return true;
	}
	return false;
}

void QHandControl::UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D )
{
	// tmp
	float fMoveTh = 10;
	float fFixZTh = -300;
	boost::chrono::milliseconds tdFixTime(100);

	// add to points list
	m_aTrackList.push_back( SHandPos( rPt2D, rPt3D ) );

	// process
	if( m_eControlStatus == NICS_STANDBY || m_eControlStatus == NICS_NO_HAND )
		UpdateStatus( NICS_WAIT_FIX );
	
	if( m_eControlStatus == NICS_WAIT_FIX )
	{
		if( rPt3D.z() < fFixZTh )
		{
			// start float hand button if fix
			if( Is2DPosFixFor( tdFixTime, fMoveTh ) )
			{
				UpdateStatus( NICS_FIXED );
				// update hand icon position
				m_pHand->resetTransform();
				m_pHand->translate( rPt2D.x() + 25, rPt2D.y() + 25 );	//TODO: Should not shift here (magic number?)
				m_pHand->show();
			}
		}
	}

	// check if hand move out from button
	if( m_eControlStatus == NICS_FIXED )
	{
		if( !m_pHand->CheckHand( rPt2D.x(), rPt2D.y() ) )
		{
			UpdateStatus( NICS_WAIT_FIX );
			m_pHand->hide();
		}
	}

	if( m_eControlStatus == NICS_INPUT )
	{
		//TODO: the control after fix
		UpdateStatus( NICS_STANDBY );
	}
}
