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

#include "CesarTest.h"
#include "scipp_ilc_utilities.h"
#include <iostream>
#include <cmath>

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


CesarTest CesarTest;

static TFile* _rootfile;
static TH2F* _hitmap;

static TH1F* _energy;
static TH1F* _radius;
static TH2F* _energyVradius;

CesarTest::CesarTest() : Processor("CesarTest") {
    // modify processor description
    _description = "Protype Processor" ;

    // register steering parameters: name, description, class-variable, default value
    registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );
}


void CesarTest::init() { 
    streamlog_out(DEBUG) << "   init called  " << std::endl ;

    _rootfile = new TFile("hitmap.root","RECREATE");
    _hitmap = new TH2F("hitmap","Hit Distribution",300.0,-150.0,150.0,300.0,-150.0,150.0);

    _energy = new TH1F("energy","Energy Distribution",300.0,0.0,260.0);
    _radius = new TH1F("radius","Radius Frequency",300.0,0.0,0.02);
    _energyVradius = new TH2F("energyVradius","Energy vs. Radius",300.0,0.0,0.02,300.0,0.0,260.0);
    // usually a good idea to
    //printParameters() ;

    _nRun = 0 ;
    _nEvt = 0 ;

}


void CesarTest::processRunHeader( LCRunHeader* run) { 
//    _nRun++ ;
} 


void CesarTest::processEvent( LCEvent * evt ) { 
    // this gets called for every event 
    // usually the working horse ...

    LCCollection* col = evt->getCollection( _colName ) ;

    // this will only be entered if the collection is available
    if( col != NULL ){
        int nElements = col->getNumberOfElements()  ;
	double radius = 0.0;
	int rad_count = 0;
        for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
	  MCParticle* hit = dynamic_cast<MCParticle*>( col->getElementAt(hitIndex) );
           const double* pos = hit->getEndpoint();

	   const double* momentum = hit->getMomentum();
	   //	   cout << "m1: " << momentum[0] << ", m2: " << momentum[1] << ", m3: " << momentum[2] << endl;
	   
	   double angle = atan ( momentum[0] / momentum[1] )

           _hitmap->Fill(pos[0],pos[1]);
	   radius = sqrt( pow(pos[0], 2.0) + pow(pos[1], 2.0) );
	   _radius->Fill(radius);

	   if(radius != 0){
	     //	     cout << "radius of particle "<< hitIndex << ": "<< radius << endl;
	     //	     cout << "xpos: "<< pos[0] << ",  ypos"<< pos[1] << endl;
	   }
	     double energy = hit->getEnergy();
	   if(radius != 0){
	     //	     cout << "energy of particle "<< hitIndex << ": "<< energy << endl;
	   }
	   _energy->Fill(energy);
	   _energyVradius->Fill(radius, energy);
        } 

    }

    _nEvt ++ ;
    cout << "event: " << _nEvt << endl;
}



void CesarTest::check( LCEvent * evt ) { 
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}



void CesarTest::end(){ 
    _rootfile->Write();
}
