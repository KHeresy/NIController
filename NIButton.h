#pragma once
#pragma region Header Files
// STL Header
#include <algorithm>
#include <array>
#include <functional>

// Boost Header
#include <boost/chrono.hpp>

// QT Header
#include <QtGui/QtGui>
#pragma endregion

/**
 * A baisc circle button with progress arc
 */
class QBaseProgressButton : public QGraphicsItem
{
public:
	std::function<void()>	m_funcPress;
	std::function<void()>	m_funcRelease;

public:
	QBaseProgressButton() : QGraphicsItem()
	{
		m_eStatus		= BS_OUTSIDE;
		m_fProgress		= 0;
		m_funcPress		= [](){};
		m_funcRelease	= [](){};

		SetSize( 70 );

		m_aColor[0] = QBrush( qRgba( 0, 128, 128, 128 ) );
		m_aColor[1] = QBrush( qRgba( 128, 128, 255, 128 ) );
		m_aColor[2] = QBrush( qRgba( 255, 0, 0, 128 ) );
	}

	virtual QRectF boundingRect() const
	{
		return m_qRect;
	}

	virtual void paint( QPainter *pPainter, const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		pPainter->setPen( QPen( qRgba( 0, 0, 0, 0 ) ) );
		pPainter->setBrush( m_aColor[m_eStatus] );
		pPainter->drawEllipse( m_qRect );

		if( m_eStatus != BS_OUTSIDE )
		{
			QPen mPen = QPen( qRgba( 255, 0, 0, 128 ) );
			mPen.setWidth(10);
			pPainter->setPen( mPen );
			pPainter->drawArc( m_qRect, 90*16, 360 * 16 * m_fProgress );
		}
	}

	virtual bool CheckInSide( const QPointF& rPos, const float& fDepth ) = 0;

	virtual void SetSize( float fSize )
	{
		float fS = fSize / 2;
		m_qRect = QRectF( -fS, -fS, fSize, fSize );
	}

protected:
	enum ESTATUS
	{
		BS_OUTSIDE,
		BS_INSIDE,
		BS_PRESSED
	};

protected:
	ESTATUS	m_eStatus;
	float	m_fProgress;

	QRectF					m_qRect;
	std::array<QBrush, 3>	m_aColor;
};

/**
 * A time-base button.
 */
class QTimerButton : public QBaseProgressButton
{
public:
	typedef boost::chrono::system_clock	TTimeColock;
	typedef boost::chrono::milliseconds	TDurationType;

	TDurationType	m_duTimeToPress;

public:
	QTimerButton() : QBaseProgressButton()
	{
		m_duTimeToPress = boost::chrono::milliseconds( 500 );
	}

	virtual bool CheckInSide( const QPointF& rPt, const float& fDepth = 0 )
	{
		if( shape().contains( mapFromScene( rPt ) ) )
		{
			switch( m_eStatus )
			{
			case BS_OUTSIDE:
				m_eStatus = BS_INSIDE;
				m_tpFirstIn = TTimeColock::now();
				break;

			case BS_INSIDE:
				m_fProgress = float( boost::chrono::duration_cast<TDurationType>( TTimeColock::now() - m_tpFirstIn ).count() ) / m_duTimeToPress.count();
				if( m_fProgress > 1 )
				{
					m_fProgress = 1;
					m_eStatus = BS_PRESSED;
					m_funcPress();
				}
				break;
			}
			return true;
		}
		else
		{
			switch( m_eStatus )
			{
			case BS_PRESSED:
				m_funcRelease();
				break;
			}
			m_eStatus	= BS_OUTSIDE;
			m_fProgress	= 0;
		}
		return false;
	}

protected:
	TTimeColock::time_point	m_tpFirstIn;
};

/**
 * A depth-base button
 */
class QDepthButton : public QBaseProgressButton
{
public:
	float	m_fPressDepth;

public:
	QDepthButton() : QBaseProgressButton()
	{
		m_fPressDepth = 50;
	}

	bool CheckInSide( const QPointF& rPt, const float& fDepth )
	{
		if( shape().contains( mapFromScene( rPt ) ) )
		{
			switch( m_eStatus )
			{
			case BS_OUTSIDE:
				m_eStatus = BS_INSIDE;
				m_fFirstInDepth = fDepth;
				break;

			case BS_INSIDE:
				m_fProgress = ComputeProgess( fDepth );
				if( m_fProgress >= 1.0f )
				{
					m_fProgress = 1;
					m_eStatus = BS_PRESSED;
					m_funcPress();
				}
				break;

			case BS_PRESSED:
				m_fProgress = ComputeProgess( fDepth );
				if( m_fProgress < 1 )
				{
					m_eStatus = BS_INSIDE;
					m_funcRelease();
				}
				break;
			}
			return true;
		}
		else
		{
			m_eStatus	= BS_OUTSIDE;
			m_fProgress	= 0;
		}
		return false;
	}

protected:
	float	m_fFirstInDepth;

protected:
	float ComputeProgess( float fDepth )
	{
		return std::min( std::max( ( m_fFirstInDepth - fDepth ) / m_fPressDepth, 0.0f ), 1.0f );
	}
};
