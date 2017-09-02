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

#include "Prediction.h"
#include "scipp_ilc_utilities.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/MCParticle.h>

#include <TFile.h>
#include <TH2D.h>
#include <MyParticle.h>
#include <Will.h>
// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"



using namespace lcio;
using namespace marlin;
using namespace std;

Prediction Prediction;

static TFile* _rootfile;
static TH2F* _prediction;
static TH1F* _vector;
static TH1F* _p_theta;
static TH1F* _e_theta;



Prediction::Prediction() : Processor("Prediction") {
    // modify processor description
    _description = "Protype Processor" ;

    // register steering parameters: name, description, class-variable, default value
    registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );

    registerProcessorParameter( "RootOutputName" , "output file"  , _root_file_name , std::string("output.root") );
}



void Prediction::init() { 
    streamlog_out(DEBUG) << "   init called  " << std::endl ;

    _rootfile = new TFile("BW_prediction.root","RECREATE");
    // usually a good idea to
    //printParameters() ;
    _prediction = new TH2F("predict", "Predicted Angle of Scatter, Correct vs Incorrect Kinematics", 1000, 0.0, 0.01, 1000, 0.0, 0.01);
    _p_theta = new TH1F("p_theta", "Theta between positron and hadronic system", 360, 0, 3.5);
    _e_theta = new TH1F("e_theta", "Theta between positron and hadronic system", 360, 0, 3.5);
    _vector = new TH1F("vector", "Vector", 200, 0.0, 0.05);
    _nEvt = 0 ;
}



void Prediction::processRunHeader( LCRunHeader* run) { 
//    _nRun++ ;
} 



void Prediction::processEvent( LCEvent * evt ) { 
    LCCollection* col = evt->getCollection( _colName ) ;
    _nEvt++;
    if( col == NULL )return;
    vector<MCParticle*> final_system;
    int stat, id =0;
    double tot_mom[]={0, 0};
    double* mom   = new double[4]();//4vec
    double* mom_e = new double[4]();
    double* mom_p = new double[4]();
    double tmom, theta, good_t, bad_t, mag, eT, pT;
    bool scatter;
    double* hadronic = new double[4]();
    double* electronic = new double[4]();

    int nElements = col->getNumberOfElements();
    scatter = false;
 
    map<int, double> max=Will::maxEnergy(col, {11, -11}, final_system);
    //Checks for scatter in electron or positron.
    for(MCParticle* particle : final_system){
      id = particle->getPDG();
      if(particle->getEnergy()==max[11]){
	//ELECTRON
	mom_e=Will::getVector(particle);
	eT = Will::getTMag(mom_e);
	if(eT!=0){
	  scatter = true;
	  electronic=Will::addVector(electronic,mom_e);
	}
      }else if(particle->getEnergy()==max[-11]){
	//POSITRON
	mom_p=Will::getVector(particle);
	pT = Will::getTMag(mom_p);
	if(pT!=0){
	  scatter = true;
	  electronic=Will::addVector(electronic,mom_p);
	}    
      }else{
	//HADRONIC
	mom=Will::getVector(particle);
	hadronic=Will::addVector(hadronic, mom);
	mag+=Will::getTMag(mom);
      }
    }
    if(scatter == true){
      //create prediction vector
      double predict[4];
      predict[0] = -hadronic[0];
      predict[1] = -hadronic[1];
      double alpha = 500 - hadronic[3] - hadronic[2];
      double beta = 500 - hadronic[3] + hadronic[2];
      

      //Given positron deflection (eBpW)
      //incorrect prediction (correct if electron deflection)
      predict[2] = -(pow(eT, 2)-pow(alpha, 2))/(2*alpha);
      //correct prediction (correct if positron deflection)
      predict[3] = (pow(pT, 2)-pow(beta, 2))/(2*beta);
      
      //Hadron transverse momentum
      double r = Will::getTMag(predict);
      
      double mag_g = sqrt(pow(predict[0], 2)+pow(predict[1], 2)+pow(predict[3], 2));
      good_t = asin(r/mag_g);

      double mag_b = sqrt(pow(predict[0], 2)+pow(predict[1], 2)+pow(predict[2], 2));
      bad_t = asin(r/mag_b);
            
      if(mag>1.0){
	_prediction->Fill(bad_t, good_t);
      }

      double dot_c = mom_p[0]*predict[0] + mom_p[1]*predict[1] + mom_p[2]*predict[3]; //Correct dot
      double dot_i = mom_p[0]*predict[0] + mom_p[1]*predict[1] + mom_p[2]*predict[2]; //Incorrect dot
      //      double e_mag = Will::getMag(mom_e);
      double p_mag = Will::getMag(mom_p);
	//      double e_mag = sqrt(pow(electronic[0], 2)+pow(electronic[1], 2)+pow(electronic[2], 2)); 
      double p_mag_c = sqrt(pow(predict[0], 2)+pow(predict[1], 2)+pow(predict[3], 2)); //Correct mag
      double p_mag_i = sqrt(pow(predict[0], 2)+pow(predict[1], 2)+pow(predict[2], 2)); //Incorrect mag
      //cout << dot_c << endl; //dot_c 59066.6
      cout << "p-mag " << p_mag << endl; //200
      cout << "p-mag_c " << p_mag_c << endl; //206
      double theta_c = acos(dot_c/(p_mag*p_mag_c)); //Correct prediction
      double theta_i = acos(dot_i/(p_mag*p_mag_i)); //Incorrect prediction
      _p_theta->Fill(alpha);
      _e_theta->Fill(beta);
      //theta = acos(dot/(e_mag*p_mag)); 
      //cout << "Prediction Efficiency :" <<  theta << endl;
    }
}

void Prediction::check( LCEvent * evt ) { 
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}



void Prediction::end(){ 
    cout << interest << endl;
    _rootfile->Write();
}
