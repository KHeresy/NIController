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
	QUserDirection( float fSize = 40 ) : QGraphicsItem()
	{
		m_qPen1.setColor( qRgb( 255, 255, 0 ) );
		m_qPen1.setWidth( 3 );

		m_qPen2.setColor( qRgb( 255, 0, 0 ) );
		m_qPen2.setWidth( 5 );

		m_vDir = QVector2D( 0, -1 );
		SetSize( fSize );
	}

	void SetSize( float fSize )
	{
		float fS = fSize / 2;
		m_qRect = QRectF( -fS, -fS, fSize, fSize );
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

		m_bUpdateransform	= true;
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

	void KeepTransform( bool bKeep = true )
	{
		m_bUpdateransform = !bKeep;
	}

public:
	std::array<QPointF,15>				m_aJoint2D;
	std::array<QVector3D,15>			m_aJointRotated;
	std::array<nite::SkeletonJoint,15>	m_aJointOri;
	QVector3D	m_vDirection;

private:
	bool		m_bUpdateransform;
	QMatrix4x4	m_qTransform;
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
		addToGroup(&m_UserDirection);

		SetSize( 640, 480 );

		m_UserSkeleton.hide();
	}

	bool Update();

	void SetSize( int w, int h )
	{
		float fDirSize = w / 16;
		m_UserDirection.SetSize( fDirSize );
		m_UserDirection.resetTransform();
		m_UserDirection.translate( w - fDirSize, h - fDirSize );

		m_UserSkeleton.m_vPositionShift = QVector2D( w / 2, h * 2.0f / 3 );
		m_UserSkeleton.m_fScale = 1.0f * w / 1600;
	}

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

	void KeepSkeletonTransform( bool bKeep )
	{
		m_UserSkeleton.KeepTransform( bKeep );
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
