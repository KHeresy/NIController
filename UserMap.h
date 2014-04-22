#pragma once

// STL Header
#include <array>

// Qt Header
#include <QtGui/QtGui>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

/**
 * The QGraphicsItem to draw user face direction
 */
class QDirection : public QGraphicsItem
{
public:
	QDirection( int uSize ) : QGraphicsItem()
	{
		m_qRect = QRectF( QPointF( 0, 0 ), QSizeF(uSize,uSize) );
		m_vDir = QVector2D( 0, -1 );
	}

	QRectF boundingRect() const
	{
		return m_qRect;
	}

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		// p1
		QPen pen( qRgb( 255, 255, 0 ) );
		pen.setWidth( 3 );
		painter->setPen( pen );
		painter->drawEllipse( m_qRect );

		// p2
		pen.setColor( qRgb( 255, 0, 0 ) );
		painter->setPen( pen );
		pen.setWidth( 5 );
		float r = m_qRect.width() / 2;
		painter->drawLine( r, r, r + r * m_vDir.x(), r + r * m_vDir.y() );
	}

public:
	QRectF		m_qRect;
	QVector2D	m_vDir;
};

/**
 * The user skeleton
 */
class QONI_Skeleton : public QGraphicsItem
{
public:
	QRectF boundingRect() const
	{
		QRectF qRect( m_aJoint2D[0], QSizeF( 1, 1 ) );
		for( auto itP = m_aJoint2D.begin(); itP != m_aJoint2D.end(); ++ itP )
			qRect |= QRectF( *itP, QSizeF( 1, 1 ) );
		return qRect;
	}

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget );

public:
	std::array<QPointF,15>	m_aJoint2D;
};

/**
 * User Map
 */
class QONI_UserMap : public QGraphicsItemGroup
{
public:
	QONI_UserMap( nite::UserTracker& rUserTracker ) : m_rUserTracker(rUserTracker)
	{
		addToGroup(&m_UserImage);
		addToGroup(&m_UserSkeleton);
		m_UserSkeleton.hide();
	}

	bool Update();

	const nite::SkeletonJoint& GetActiveUserJoint( const nite::JointType& eJoint ) const 
	{
		return m_aJoint3D[eJoint];
	}

	const QPointF& GetActiveUserJoint2D( const nite::JointType& eJoint ) const 
	{
		return m_aJoint2D[eJoint];
	}

	QRectF boundingRect() const
	{
		return m_UserImage.boundingRect();
	}

protected:
	void LoadSkeleton( const nite::Skeleton& rSkeleton );

public:
	nite::UserTracker&		m_rUserTracker;
	QGraphicsPixmapItem		m_UserImage;
	QONI_Skeleton			m_UserSkeleton;

	std::array<nite::SkeletonJoint,15>	m_aJoint3D;
	std::array<QPointF,15>				m_aJoint2D;
};
