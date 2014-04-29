#pragma region Header Files

// STL Header
#include <array>
#include <iostream>
#include <sstream>

// Boost Header
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

// Application header
#include "UserMap.h"
#include "HandControl.h"
#include "NIControl.h"

#pragma endregion

QSettings*	g_pSetting;

int main( int argc, char** argv )
{
	#pragma region Qt Core
	// Qt Application
	QApplication qOpenNIApp( argc, argv );
	#pragma endregion

	#pragma region Qt Widget
	// Qt Window
	QNIControl qWin(false);
	qWin.InitialNIDevice( 640, 480 );
	qWin.show();
	qWin.resize( 640, 480 );
	#pragma endregion

	// main loop
	qWin.Start();
	return qOpenNIApp.exec();
}
