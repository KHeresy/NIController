#pragma region Header Files

// Application header
#include "NIControl.h"

#pragma endregion

int main( int argc, char** argv )
{
	#pragma region options
	//TODO: should load from file
	int	iW = 640,
		iH = 480;
	#pragma endregion

	#pragma region Qt Core
	// Qt Application
	QApplication qOpenNIApp( argc, argv );
	#pragma endregion

	#pragma region Qt Widget
	// Qt Window
	QNIControl qWin(false);
	qWin.InitialNIDevice( iW, iH );
	qWin.SetSkeletonSmoothing( 0.75f );	//TODO: should handle from option
	qWin.resize( 640, 480 );
	qWin.show();

	qWin.m_fJointConfidence;	//TODO: should assign from option
	#pragma endregion

	// main loop
	qWin.Start();
	return qOpenNIApp.exec();
}
