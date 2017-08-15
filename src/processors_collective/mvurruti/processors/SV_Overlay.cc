#undef _GLIBCXX_USE_CXX11_ABI
#define _GLIBCXX_USE_CXX11_ABI 0
/* 
 * Ok, so I like C++11. Unfortunately,
 * Marlin is built with ansi C, so the processor
 * constructor freaks out about the string that is
 * passed to it as an argument. The above two lines
 * fix that issue, allowing our code to be compatible
 * with ansi C class declarations.
 * Big thanks to Daniel Bittman for helping me fix this.
 */

/*
 * author Christopher Milke
 * April 5, 2016
 */

#include "SV_Overlay.h"
#include "scipp_ilc_utilities.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/MCParticle.h>

#include <TFile.h>
#include <TH2D.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"



using namespace lcio;
using namespace marlin;
using namespace std;
using namespace scipp_ilc;

SV_Overlay SV_Overlay;

static TFile* _rootfile;
static TH1D* _S;
static TH1D* _V;
static TH1D* _M;
static bool v = false;


// below is a prototype. This tells the program there is a print statement that we want to implement through the code even though the if statement is all the way to the end
void print (string input="") ;

SV_Overlay::SV_Overlay() : Processor("SV_Overlay") {
	// modify processor description
	_description = "Protype Processor" ;

	// register steering parameters: name, description, class-variable, default value
	registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );

	registerProcessorParameter( "RootOutputName" , "output file"  , _root_file_name , std::string("output.root") );
}



void SV_Overlay::init() { 
	streamlog_out(DEBUG) << "   init called  " << std::endl ;

	_rootfile = new TFile("SV_Overlay.root","RECREATE");
	_S = new TH1D("S", "That S thing whose name I forgot", 200, 0.0, 20.0);
	_V = new TH1D("V", "That V thing whose name I also forgot", 200, 0.0, 20.0); 
	_M = new TH1D("M", "That M thing whose name I also forgot", 200, 0.0, 20.0); 

	// usually a good idea to
	//printParameters() ;
	_nEvt = 0 ;

	cout << "Anything to let me know " << endl;

}



void SV_Overlay::processRunHeader( LCRunHeader* run) { 
	//    _nRun++ ;
} 



void SV_Overlay::processEvent( LCEvent * evt ) { 
	// this gets called for every event 
	// usually the working horse ...
	LCCollection* col = evt->getCollection( _colName ) ;

	double tot_mom[]={0, 0};
	double S=0,V=0,M=0; 
	double stat;
	// this will only be entered if the collection is available
	if( col != NULL ){
		int nElements = col->getNumberOfElements()  ;
		print();
		print();
		print("=====EVENT: " + to_string( _nEvt ) + " ===== " );
		vector<MCParticle*> system;

		for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
			MCParticle* hit = dynamic_cast<MCParticle*>( col->getElementAt(hitIndex) );


			stat = hit->getGeneratorStatus();
			if(stat==1){ 


				double mom[2];
				mom[0] = hit->getMomentum()[0]; 
				mom[1] = hit->getMomentum()[1];

				double mag = sqrt(pow(mom[0], 2)+pow(mom[1], 2));
				S+=mag;

				M+=hit->getMass();
				
				tot_mom[0]+=mom[0];
				tot_mom[1]+=mom[1];


			}//end final state
		}//end for
		//for(MCParticle* particle : system)



		V=sqrt(pow(tot_mom[0], 2)+pow(tot_mom[1], 2));


		_S->Fill(S); 

		_V->Fill(V);

		_M->Fill(M);
	}

	_nEvt ++ ;
}



void SV_Overlay::check( LCEvent * evt ) { 
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}



void SV_Overlay::end(){ 
	_rootfile->Write();
}

void print (string input) {
  if (v) cout << input << endl;
}

