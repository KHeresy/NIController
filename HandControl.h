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
	typedef boost::chrono::system_clock::time_point TTimePoint;

	enum EControlStatus
	{
		NICS_STANDBY,
		NICS_WAIT_FIX,
		NICS_FIXED,
		NICS_INPUT,
	};

public:
	QHandControl()
	{
		m_eControlStatus	= NICS_STANDBY;
		m_aTrackList.set_capacity( 150 );

		m_pHand		= new QAbsNIButton( 100 );
		QHandControl* pThis = this;
		m_pHand->m_mFunc = [pThis](){
			pThis->m_eControlStatus = QHandControl::NICS_INPUT;
		};
		m_pHand->hide();
	}

	void ResetList()
	{
		m_eControlStatus	= NICS_STANDBY;
		m_aTrackList.clear();
		m_pHand->hide();
	}

	TTimePoint UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D );

	template<typename _TDuration>
	bool IsFixFor( const QPointF& rPt, const TTimePoint& rTP, const _TDuration& rDuration, float fMoveRange = 10 )
	{
		for( auto itPt = m_aTrackList.rbegin(); itPt != m_aTrackList.rend(); ++ itPt )
		{
			// check position
			if( QLineF( rPt, itPt->second ).length() > fMoveRange )
				return false;

			// check time
			if( rTP - itPt->first > rDuration )
				return true;
		}
		return false;
	}

public:
	QAbsNIButton*	m_pHand;


private:
	EControlStatus	m_eControlStatus;

	QPointF		m_curPos2D;
	QVector3D	m_curPos3D;
	boost::circular_buffer< std::pair<TTimePoint, QPointF> >	m_aTrackList;
};

QHandControl::TTimePoint QHandControl::UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D )
{
	// tmp
	float fMoveTh = 10;
	float fFixZTh = -300;
	boost::chrono::milliseconds tdFixTime(100);

	m_curPos2D = rPt2D;
	m_curPos3D = rPt3D;

	// add to points list
	TTimePoint tpNow = boost::chrono::system_clock::now();
	m_aTrackList.push_back( std::make_pair( tpNow, rPt2D ) );

	// process
	if( m_eControlStatus == NICS_STANDBY )
		m_eControlStatus = NICS_WAIT_FIX;
	
	if( m_eControlStatus == NICS_WAIT_FIX )
	{
		if( m_curPos3D.z() < fFixZTh )
		{
			// start float hand button if fix
			if( IsFixFor( rPt2D, tpNow, tdFixTime, fMoveTh ) )
			{
				m_eControlStatus = NICS_FIXED;
				// update hand icon position
				m_pHand->resetTransform();
				m_pHand->translate( m_curPos2D.x() + 25, m_curPos2D.y() + 25 );	//TODO: Should not shift here (magic number?)
				m_pHand->show();
			}
		}
	}

	// check if hand move out from button
	if( m_eControlStatus == NICS_FIXED )
	{
		if( !m_pHand->CheckHand( m_curPos2D.x(), m_curPos2D.y() ) )
		{
			m_eControlStatus = NICS_WAIT_FIX;
			m_pHand->hide();
		}
	}

	if( m_eControlStatus == NICS_INPUT )
	{
		//TODO: the control after fix
		m_eControlStatus	= NICS_STANDBY;
	}

	return tpNow;
}
