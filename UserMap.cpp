#include "UserMap.h"

void QONI_Skeleton::paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
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
	for( int i = 0; i < m_aJoint2D.size(); ++ i )
	{
		float fD = m_aJointRotated[i].z();
		if( fD > 0 )
		{
			painter->setPen( pen );
		}
		else
		{
			fD = min( 1.0f, -fD / 500 );
			QPen pen1( qRgba( fD * 255, fD * 255, 64, 255 ) );
			pen1.setWidth( 3 );
			painter->setPen( pen1 );
		}
		painter->drawEllipse( m_aJoint2D[i], 5, 5 );
	}
}

void QONI_Skeleton::SetSkeleton( const nite::Skeleton& rSkeleton )
{
	#pragma region Load all joints
	m_aJointOri[ 0] = rSkeleton.getJoint(nite::JOINT_HEAD			);
	m_aJointOri[ 1] = rSkeleton.getJoint(nite::JOINT_NECK			);
	m_aJointOri[ 2] = rSkeleton.getJoint(nite::JOINT_LEFT_SHOULDER	);
	m_aJointOri[ 3] = rSkeleton.getJoint(nite::JOINT_RIGHT_SHOULDER	);
	m_aJointOri[ 4] = rSkeleton.getJoint(nite::JOINT_LEFT_ELBOW		);
	m_aJointOri[ 5] = rSkeleton.getJoint(nite::JOINT_RIGHT_ELBOW	);
	m_aJointOri[ 6] = rSkeleton.getJoint(nite::JOINT_LEFT_HAND		);
	m_aJointOri[ 7] = rSkeleton.getJoint(nite::JOINT_RIGHT_HAND		);
	m_aJointOri[ 8] = rSkeleton.getJoint(nite::JOINT_TORSO			);
	m_aJointOri[ 9] = rSkeleton.getJoint(nite::JOINT_LEFT_HIP		);
	m_aJointOri[10] = rSkeleton.getJoint(nite::JOINT_RIGHT_HIP		);
	m_aJointOri[11] = rSkeleton.getJoint(nite::JOINT_LEFT_KNEE		);
	m_aJointOri[12] = rSkeleton.getJoint(nite::JOINT_RIGHT_KNEE		);
	m_aJointOri[13] = rSkeleton.getJoint(nite::JOINT_LEFT_FOOT		);
	m_aJointOri[14] = rSkeleton.getJoint(nite::JOINT_RIGHT_FOOT		);
	#pragma endregion

	#pragma region Compute transformation
	// compute face direction
	auto tr = m_aJointOri[8].getOrientation();
	QQuaternion qTRotation( tr.w, tr.x, tr.y, tr.z );
	m_vDirection = qTRotation.rotatedVector( QVector3D( 0, 0, -1 ) );

	// compute transformation matrix
	QMatrix4x4 qTransform;
	qTransform.setToIdentity();
	auto tt = m_aJointOri[8].getPosition();
	qTransform.translate( tt.x, tt.y, tt.z );
	qTransform.rotate( qTRotation );
	qTransform = qTransform.inverted();
	#pragma endregion
	
	for( int i = 0; i < m_aJointRotated.size(); ++ i )
	{
		const auto& rPos = m_aJointOri[i].getPosition();
		QVector4D qPos( rPos.x, rPos.y, rPos.z, 1 );
		m_aJointRotated[i] = ( qTransform * qPos ).toVector3D();
		m_aJoint2D[i] = QPointF( 320 + m_aJointRotated[i].x() / 5, 240 - m_aJointRotated[i].y() / 5 );
	}
}

bool QONI_UserMap::Update()
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
				m_UserSkeleton.SetSkeleton( rSkeleton );
				m_UserDirection.m_vDir = QVector2D( m_UserSkeleton.m_vDirection.x(), m_UserSkeleton.m_vDirection.z() ).normalized();
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
