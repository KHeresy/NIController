#pragma region Header Files

// Application header
#include "NIControl.h"

#pragma endregion

int main( int argc, char** argv )
{
	#pragma region Qt Core
	// Qt Application
	QApplication qOpenNIApp( argc, argv );
	#pragma endregion

	#pragma region Qt Widget
	QString sINIFile = "";
	if( argc > 1 )
		sINIFile = argv[1];
	else
		sINIFile = "NIController.ini";

	// Qt Window
	QNIControl qWin( sINIFile );
	qWin.InitialNIDevice();
	qWin.show();

	qWin.m_fJointConfidence;	//TODO: should assign from option
	#pragma endregion

	// main loop
	qWin.Start();
	return qOpenNIApp.exec();
}
