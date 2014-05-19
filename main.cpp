// Application header
#include "NIControl.h"

int main( int argc, char** argv )
{
	// Qt Application
	QApplication qOpenNIApp( argc, argv );

	// check if assign a configuration file
	QString sINIFile = "";
	if( argc > 1 )
		sINIFile = argv[1];
	else
		sINIFile = "NIController.ini";

	// Qt Window
	QNIControl qWin( sINIFile );
	if( qWin.InitialNIDevice() )
	{
		qWin.show();

		// main loop
		qWin.Start();
		return qOpenNIApp.exec();
	}
}
