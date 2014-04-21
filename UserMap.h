#pragma once

// STL Header
#include <array>

// Qt Header
#include <QtGui/QtGui>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

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

	void paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		// set pen for drawing
		QPen pen( qRgba( 64, 64, 255, 255 ) );
		pen.setWidth( 3 );
		painter->setPen( pen );

		// draw head
		painter->drawLine( m_aJoint2D[0], m_aJoint2D[1] );

		// draw body
		painter->drawLine( m_aJoint2D[1], m_aJoint2D[2] );
		painter->drawLine( m_aJoint2D[1], m_aJoint2D[3] );
		painter->drawLine( m_aJoint2D[1], m_aJoint2D[8] );
		painter->drawLine( m_aJoint2D[8], m_aJoint2D[9] );
		painter->drawLine( m_aJoint2D[8], m_aJoint2D[10] );

		// hands
		painter->drawLine( m_aJoint2D[2], m_aJoint2D[4] );
		painter->drawLine( m_aJoint2D[4], m_aJoint2D[6] );
		painter->drawLine( m_aJoint2D[3], m_aJoint2D[5] );
		painter->drawLine( m_aJoint2D[5], m_aJoint2D[7] );

		// legs
		painter->drawLine( m_aJoint2D[9], m_aJoint2D[11] );
		painter->drawLine( m_aJoint2D[11], m_aJoint2D[13] );
		painter->drawLine( m_aJoint2D[10], m_aJoint2D[12] );
		painter->drawLine( m_aJoint2D[12], m_aJoint2D[14] );

		// draw joints
		for( auto itP = m_aJoint2D.begin(); itP != m_aJoint2D.end(); ++ itP )
			painter->drawEllipse( *itP, 5, 5 );
	}

public:
	std::array<QPointF,15>	m_aJoint2D;
};

class QONI_UserMap : public QGraphicsItemGroup
{
public:
	QONI_UserMap( nite::UserTracker& rUserTracker ) : m_rUserTracker(rUserTracker)
	{
		addToGroup(&m_UserImage);
		addToGroup(&m_UserSkeleton);
		m_UserSkeleton.hide();
	}

	bool Update()
	{
		nite::UserTrackerFrameRef	vfUserFrame;
		if( m_rUserTracker.readFrame( &vfUserFrame ) == nite::STATUS_OK )
		{
			// get user data
			const nite::Array<nite::UserData>& aUsers = vfUserFrame.getUsers();

			// get depth map
			openni::VideoFrameRef vfDepth = vfUserFrame.getDepthFrame();
			int w = vfDepth.getWidth(),
				h = vfDepth.getHeight();
			const openni::DepthPixel* pDepth = static_cast<const openni::DepthPixel*>( vfDepth.getData() );

			QImage mImage( w, h, QImage::Format_ARGB32 );
			bool	bUseUserMap	= false;
			if( aUsers.getSize() > 0 )
			{
				// scan user for tracking skeleton and find active user
				const nite::UserData*	pActiveUser = NULL;
				float fDistance = 100000;
				for( int i = 0; i < aUsers.getSize(); ++ i )
				{
					const nite::UserData& rUser = aUsers[i];
					if( rUser.isNew() )
					{
						m_rUserTracker.startSkeletonTracking( rUser.getId() );
					}
					else
					{
						const nite::Skeleton& rSkeleton = rUser.getSkeleton();
						if( rSkeleton.getState() == nite::SKELETON_TRACKED )
						{
							if( rUser.getCenterOfMass().z < fDistance )
							{
								fDistance = rUser.getCenterOfMass().z;
								pActiveUser = &rUser;
							}
						}
					}
				}

				if( pActiveUser != NULL )
				{
					bUseUserMap = true;

					// get user map
					const nite::UserMap& rUserMap = vfUserFrame.getUserMap();
					const nite::UserId* pUserMap = rUserMap.getPixels();
					nite::UserId uID = pActiveUser->getId();

					// draw user map
					//#pragma omp parallel for num_threads(4)
					for( int y = 0; y < h; ++ y )
					{
						for( unsigned int x = 0; x < w; ++ x )
						{
							unsigned int uIdx = x+w*y;
							if( pUserMap[uIdx] == uID )
							{
								const openni::DepthPixel& rValue = pDepth[x+w*y];
								int iColor = 128 * ( 1.0f - 1.0f * rValue / 3000 ) + 127;
								mImage.setPixel( x, y, qRgba( iColor, 0, 0, 128 ) );
							}
							else
							{
								mImage.setPixel( x, y, qRgba( 0, 0, 0, 0 ) );
							}
						}
					}

					// Analyze user skeleton
					const auto& rSkeleton = pActiveUser->getSkeleton();
					LoadSkeleton( rSkeleton );
					m_UserSkeleton.show();
				}
			}

			if( !bUseUserMap )
			{
				//#pragma omp parallel for num_threads(4)
				for( int y = 0; y < h; ++ y )
				{
					for( unsigned int x = 0; x < w; ++ x )
					{
						const openni::DepthPixel& rValue = pDepth[x+w*y];
						int iColor = 255 * ( 1.0f - 1.0f * ( rValue - 1000 ) / 5000 );
						mImage.setPixel( x, y, qRgba( iColor, iColor, iColor, iColor ) );
					}
				}
			}

			m_UserImage.setPixmap( QPixmap::fromImage( mImage ) );

			return bUseUserMap;
		}
	}

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
	void LoadSkeleton( const nite::Skeleton& rSkeleton )
	{
		m_aJoint3D[ 0] = rSkeleton.getJoint(nite::JOINT_HEAD			);
		m_aJoint3D[ 1] = rSkeleton.getJoint(nite::JOINT_NECK			);
		m_aJoint3D[ 2] = rSkeleton.getJoint(nite::JOINT_LEFT_SHOULDER	);
		m_aJoint3D[ 3] = rSkeleton.getJoint(nite::JOINT_RIGHT_SHOULDER	);
		m_aJoint3D[ 4] = rSkeleton.getJoint(nite::JOINT_LEFT_ELBOW		);
		m_aJoint3D[ 5] = rSkeleton.getJoint(nite::JOINT_RIGHT_ELBOW		);
		m_aJoint3D[ 6] = rSkeleton.getJoint(nite::JOINT_LEFT_HAND		);
		m_aJoint3D[ 7] = rSkeleton.getJoint(nite::JOINT_RIGHT_HAND		);
		m_aJoint3D[ 8] = rSkeleton.getJoint(nite::JOINT_TORSO			);
		m_aJoint3D[ 9] = rSkeleton.getJoint(nite::JOINT_LEFT_HIP		);
		m_aJoint3D[10] = rSkeleton.getJoint(nite::JOINT_RIGHT_HIP		);
		m_aJoint3D[11] = rSkeleton.getJoint(nite::JOINT_LEFT_KNEE		);
		m_aJoint3D[12] = rSkeleton.getJoint(nite::JOINT_RIGHT_KNEE		);
		m_aJoint3D[13] = rSkeleton.getJoint(nite::JOINT_LEFT_FOOT		);
		m_aJoint3D[14] = rSkeleton.getJoint(nite::JOINT_RIGHT_FOOT		);

		for( int i = 0; i < m_aJoint3D.size(); ++ i )
		{
			const auto& rPos = m_aJoint3D[i].getPosition();
			float x, y;
			m_rUserTracker.convertJointCoordinatesToDepth( rPos.x, rPos.y, rPos.z, &x, &y );
			m_aJoint2D[i] = QPointF( x, y );
			m_UserSkeleton.m_aJoint2D[i] = m_aJoint2D[i];
		}
	}

public:
	nite::UserTracker&		m_rUserTracker;
	QGraphicsPixmapItem		m_UserImage;
	QONI_Skeleton			m_UserSkeleton;

	std::array<nite::SkeletonJoint,15>	m_aJoint3D;
	std::array<QPointF,15>				m_aJoint2D;
};
