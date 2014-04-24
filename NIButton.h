#pragma once

// STL Header
#include <algorithm>
#include <array>
#include <functional>

// Boost header
#include <boost/chrono.hpp>

// Qt Header
#include <QtGui/QtGui>

class QAbsNIButton : public QGraphicsItem
{
protected:
	enum BUTTON_STATE
	{
		OUT_SIDE,
		IN_SIDE,
		PRESSED
	};

public:
	std::function<void()>		m_mFunc;	// callback function
	boost::chrono::milliseconds	m_tTimeToHold;

public:
	QAbsNIButton( unsigned int uSize ) : QGraphicsItem()
	{
		m_tTimeToHold = boost::chrono::milliseconds(1000);
		float fS	= float(uSize) / 2;
		m_Rect		= QRectF( -fS, -fS, fS, fS );
		m_eState	= OUT_SIDE;
		m_fProgress	= 0.0f;
		m_mFunc		= [](){};

		m_aColor[0] = QBrush( qRgba( 0, 128, 128, 32 ) );
		m_aColor[1] = QBrush( qRgba( 128, 128, 255, 16 ) );
		m_aColor[2] = QBrush( qRgba( 255, 0, 0, 128 ) );
	}

	QRectF boundingRect() const
	{
		return m_Rect;
	}

	void paint( QPainter *pPainter, const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		pPainter->setPen( QPen( qRgba( 0, 0, 0, 0 ) ) );
		pPainter->setBrush( m_aColor[m_eState] );
		pPainter->drawEllipse( m_Rect );

		if( m_eState != OUT_SIDE )
		{
			QPen mPen = QPen( qRgba( 255, 0, 0, 128 ) );
			mPen.setWidth(10);
			pPainter->setPen( mPen );
			pPainter->drawArc( m_Rect, 90*16, 360 * 16 * m_fProgress );
		}
	}

	bool CheckHand( int x, int y )
	{
		if( sceneBoundingRect().contains( x, y ) )
		{
			if( m_eState == OUT_SIDE )
			{
				m_timeFirstIn = boost::chrono::system_clock::now();
				m_eState = IN_SIDE;
			}
			else
			{
				m_fProgress = float(boost::chrono::duration_cast<boost::chrono::milliseconds>( boost::chrono::system_clock::now() - m_timeFirstIn ).count()) / m_tTimeToHold.count();
				if( m_eState == IN_SIDE )
				{
					if( m_fProgress >= 1 )
						m_eState = PRESSED;
				}
				else if( m_eState == PRESSED )
				{
					if( m_fProgress < 1 )
					{
						m_mFunc();
						m_eState = IN_SIDE;
					}
				}
			}
			return true;
		}
		else
		{
			m_eState = OUT_SIDE;
			return false;
		}
	}

protected:
	QRectF			m_Rect;
	BUTTON_STATE	m_eState;		// current state
	float			m_fProgress;	// current button progress ( 0 - 1 )

	boost::chrono::system_clock::time_point	m_timeFirstIn;
	std::array<QBrush, 3>	m_aColor;
};
