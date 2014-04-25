#include "HandControl.h"

void QHandIcon::paint( QPainter *pPainter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	switch( m_eStatus )
	{
	case HS_GENERAL:
		{
			QPen mPen = QPen( qRgba( 255, 255, 0, 128 ) );
			mPen.setWidth( 5 );
			pPainter->setPen( mPen );
			pPainter->drawArc( m_Rect, 0, 360 * 16 );
		}
		break;

	case HS_FIXING:
		{
			QPen mPen1 = QPen( qRgba( 0, 255, 0, 255 ) );
			mPen1.setWidth(15);
			pPainter->setPen( mPen1 );
			pPainter->drawArc( m_Rect, 90*16, 360 * 16 * m_fProgress );

			QPen mPen = QPen( qRgba( 255, 255, 0, 255 ) );
			mPen.setWidth( 5 );
			pPainter->setPen( mPen );
			pPainter->drawArc( m_Rect, 0, 360 * 16 );
		}
		break;

	case HS_FIXED:
		{
			pPainter->setPen( QPen( qRgba( 0, 0, 0, 0 ) ) );
			pPainter->setBrush( QBrush( qRgba( 255, 0, 0, 128 ) ) );
			pPainter->drawEllipse( m_Rect );
		}
		break;
	}
}

bool QHandControl::UpdateStatus( const QHandControl::EControlStatus& eStatus )
{
	if( m_eControlStatus != eStatus )
	{
		m_eControlStatus = eStatus;
		m_HandIcon.show();

		switch( m_eControlStatus )
		{
		case NICS_NO_HAND:
			HandReset();
			m_HandIcon.hide();
			break;

		case NICS_STANDBY:
			m_HandIcon.m_eStatus = QHandIcon::HS_GENERAL;
			break;

		case NICS_FIXING:
			m_HandIcon.m_eStatus = QHandIcon::HS_FIXING;
			m_FixPos = CurrentPos();
			break;

		case NICS_FIXED:
			m_FixPos = CurrentPos();
			m_HandIcon.m_eStatus = QHandIcon::HS_FIXED;
			break;
		}
		return true;
	}
	return false;
}

void QHandControl::UpdateHandPoint( const QPointF& rPt2D, const QVector3D& rPt3D )
{
	//TODO: tmp only
	float fMoveTh = 10;
	float fFixZTh = -300;
	boost::chrono::milliseconds tdPreFixTime(100);
	boost::chrono::milliseconds tdFixTime(1000);

	// add to points list
	SHandPos mPos( rPt2D, rPt3D );
	m_aTrackList.push_back( mPos );

	// move hand icon
	m_HandIcon.resetTransform();
	m_HandIcon.translate( rPt2D.x(), rPt2D.y() );

	// process
	if( m_eControlStatus == NICS_NO_HAND )
		UpdateStatus( NICS_STANDBY );
	
	if( m_eControlStatus == NICS_STANDBY )
	{
		if( rPt3D.z() < fFixZTh )
		{
			// start float hand button if fix
			if( Is2DPosFixFor( tdPreFixTime, fMoveTh ) )
			{
				UpdateStatus( NICS_FIXING );
			}
		}
	}

	// check if hand move out from button
	if( m_eControlStatus == NICS_FIXING )
	{
		if( QLineF( m_FixPos.mPos2D, rPt2D ).length() > fMoveTh )
		{
			UpdateStatus( NICS_STANDBY );
		}
		else
		{
			m_HandIcon.m_fProgress = float(boost::chrono::duration_cast<boost::chrono::milliseconds>( mPos.tpTime - m_FixPos.tpTime ).count()) / tdFixTime.count();
			if( m_HandIcon.m_fProgress > 1 )
			{
				UpdateStatus( NICS_FIXED );
			}
		}
	}

	if( m_eControlStatus == NICS_INPUT )
	{
		//TODO: the control after fix
		UpdateStatus( NICS_NO_HAND );
	}
}
