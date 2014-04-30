#pragma once
#pragma region Header Files
// STL Header
#include <array>

// Qt Header
#include <QtGui/QtGui>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

#pragma endregion

/**
 * The QGraphicsItem to draw user face direction
 */
class QUserDirection : public QGraphicsItem
{
public:
	QPen	m_qPen1;
	QPen	m_qPen2;

public:
	QUserDirection( int uSize ) : QGraphicsItem()
	{
		m_qPen1.setColor( qRgb( 255, 255, 0 ) );
		m_qPen1.setWidth( 3 );

		m_qPen2.setColor( qRgb( 255, 0, 0 ) );
		m_qPen2.setWidth( 5 );

		float fS = 1.0f * uSize / 2;
		m_qRect = QRectF( -fS, -fS, uSize, uSize );
		m_vDir = QVector2D( 0, -1 );
	}

	QRectF boundingRect() const
	{
		return m_qRect;
	}

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		// p1
		painter->setPen( m_qPen1 );
		painter->drawEllipse( m_qRect );

		// p2
		painter->setPen( m_qPen2 );
		float r = m_qRect.width() / 2;
		painter->drawLine( 0, 0, r * m_vDir.x(), r * m_vDir.y() );
	}

	void SetDirection( const QVector2D& rVec )
	{
		m_vDir = rVec;
	}

private:
	QRectF		m_qRect;
	QVector2D	m_vDir;
};

/**
 * The user skeleton
 */
class QONI_Skeleton : public QGraphicsItem
{
public:
	float		m_fScale;
	QVector2D	m_vPositionShift;
	QPen		m_qSkeletonPen;

public:
	QONI_Skeleton()
	{
		m_fScale			= 1.0f / 2.5f;
		m_vPositionShift	= QVector2D( 320, 320 );

		m_qSkeletonPen.setWidth( 3 );
		m_qSkeletonPen.setColor( qRgba( 64, 64, 255, 192 ) );
	}

	QRectF boundingRect() const
	{
		QRectF qRect( m_aJoint2D[0], QSizeF( 1, 1 ) );
		for( auto itP = m_aJoint2D.begin(); itP != m_aJoint2D.end(); ++ itP )
			qRect |= QRectF( *itP, QSizeF( 1, 1 ) );
		return qRect;
	}

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget );

	void SetSkeleton( const nite::Skeleton& rSkeleton );

public:
	std::array<QPointF,15>				m_aJoint2D;
	std::array<QVector3D,15>			m_aJointRotated;
	std::array<nite::SkeletonJoint,15>	m_aJointOri;
	QVector3D	m_vDirection;
};

/**
 * User Map
 */
class QONI_UserMap : public QGraphicsItemGroup
{
public:
	QONI_UserMap( nite::UserTracker& rUserTracker ) : m_rUserTracker(rUserTracker), m_UserDirection(40)
	{
		addToGroup(&m_UserImage);
		addToGroup(&m_UserSkeleton);
		addToGroup(&m_UserDirection);
		m_UserDirection.translate( 590, 430 );
		m_UserSkeleton.hide();
	}

	bool Update();

	const nite::SkeletonJoint& GetActiveUserJoint( const nite::JointType& eJoint ) const 
	{
		return m_UserSkeleton.m_aJointOri[eJoint];
	}

	const QVector3D& GetActiveUserJointTR( const nite::JointType& eJoint ) const 
	{
		return m_UserSkeleton.m_aJointRotated[eJoint];
	}

	const QPointF& GetActiveUserJoint2D( const nite::JointType& eJoint ) const 
	{
		return m_UserSkeleton.m_aJoint2D[eJoint];
	}

	QRectF boundingRect() const
	{
		return m_UserImage.boundingRect();
	}

private:
	nite::UserTracker&		m_rUserTracker;
	QGraphicsPixmapItem		m_UserImage;
	QONI_Skeleton			m_UserSkeleton;
	QUserDirection			m_UserDirection;
};
