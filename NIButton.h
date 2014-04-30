#pragma once

// STL Header
#include <algorithm>
#include <array>
#include <functional>

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
	std::function<void()>	m_mFunc;	// callback function
	int						m_iPressDepth;

public:
	QAbsNIButton( unsigned int uSize ) : QGraphicsItem()
	{
		m_iPressDepth	= 75;

		float fS	= float(uSize) / 2;
		m_Rect		= QRectF( -fS, -fS, uSize, uSize );
		m_eState	= OUT_SIDE;
		m_fProgress	= 0.0f;
		m_mFunc		= [](){};

		m_aColor[0] = QBrush( qRgba( 0, 128, 128, 128 ) );
		m_aColor[1] = QBrush( qRgba( 128, 128, 255, 128 ) );
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

	bool CheckHand( int x, int y, int z )
	{
		if( sceneBoundingRect().contains( x, y ) )
		{
			m_fProgress = float( min( max( m_iInitDepth - z, 0 ), m_iPressDepth ) ) / m_iPressDepth;

			if( m_eState == OUT_SIDE )
			{
				m_iInitDepth = z;
				m_eState = IN_SIDE;
			}
			else if( m_eState == IN_SIDE )
			{
				if( m_fProgress == 1 )
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
	int				m_iInitDepth;
	BUTTON_STATE	m_eState;		// current state
	float			m_fProgress;	// current button progress ( 0 - 1 )

	std::array<QBrush, 3>	m_aColor;
};
