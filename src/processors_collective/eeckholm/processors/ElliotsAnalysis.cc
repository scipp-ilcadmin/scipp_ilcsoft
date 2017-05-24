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

#include "ElliotsAnalysis.h"
#include "scipp_ilc_utilities.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/MCParticle.h>

#include <TFile.h>
#include <TH2D.h>
#include <math.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"



using namespace lcio;
using namespace marlin;
using namespace std;


ElliotsAnalysis ElliotsAnalysis;

static TFile* _rootfile;
static TH2F* _hitmap;


int numEvents = 8;

double* pX = new double[numEvents];
double* pY = new double[numEvents];
double* nX = new double[numEvents];
double* nY = new double[numEvents];
double* peventBarycenterX = new double[numEvents];
double* peventBarycenterY = new double[numEvents];
double* neventBarycenterX = new double[numEvents];
double* neventBarycenterY = new double[numEvents];
double* pEnergyDep = new double[numEvents];
double* nEnergyDep = new double[numEvents];
double* pLR = new double[numEvents];
double* nLR = new double[numEvents];
double* pTD = new double[numEvents];
double* nTD = new double[numEvents];
double* pmeanDepth = new double[numEvents];
double* nmeanDepth = new double[numEvents];
double* prmoment = new double[numEvents];
double* nrmoment = new double[numEvents]; 
double* pinvrmoment = new double[numEvents];
double* ninvrmoment = new double[numEvents];


int currentEvent = 0;

ElliotsAnalysis::ElliotsAnalysis() : Processor("ElliotsAnalysis") {
    // modify processor description
    _description = "Protype Processor" ;

    // register steering parameters: name, description, class-variable, default value
    registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );

    registerProcessorParameter( "RootOutputName" , "output file"  , _root_file_name , std::string("output.root") );
}



void ElliotsAnalysis::init() { 
    streamlog_out(DEBUG) << "   init called  " << std::endl ;

    _rootfile = new TFile("Elliothitmap.root","RECREATE");
    _hitmap = new TH2F("Elliothitmap","Hit Distribution",300.0,-150.0,150.0,300.0,-150.0,150.0);

    // usually a good idea to
    //printParameters() ;

    

    _nRun = 0 ;
    _nEvt = 0 ;

}



void ElliotsAnalysis::processRunHeader( LCRunHeader* run) { 
//    _nRun++ ;
} 

//pEnerygDep, nEnergyDep, pLR, nLR, pTD, nTD, pmeanDepth, nmeanDepth, prmoment, nrmoment, pinvrmoment, ninvrmoment, pX, pY, nX, nY   
double* ElliotsAnalysis::calculateObservables(LCCollection* col, double barycenters[4]){

  double ptotalEnergy = 0, ntotalEnergy = 0;
  double pLR = 0, nLR = 0, pTD = 0, nTD = 0;
  double pmeanDepth = 0, nmeanDepth = 0;
  double prmoment = 0, nrmoment = 0;
  double pinvrmoment = 0, ninvrmoment = 0;
  double pX = 0, pY = 0, nX = 0, nY = 0;

  //LR
  double pnum_LR = 0, pdenom_LR = 0, nnum_LR = 0, ndenom_LR = 0;
  //TD
  double pnum_TD = 0, pdenom_TD = 0, nnum_TD = 0, ndenom_TD = 0; 
  //meanDepth
  double pnum_meanDepth = 0, nnum_meanDepth = 0;
  //r-moment
  double pnum_moment = 0, pdenom_moment = 0, nnum_moment = 0, ndenom_moment = 0;
  //invr-moment
  double pnum_invrmoment = 0, nnum_invrmoment = 0;
  
  double prad = 0, nrad = 0;
 
  if( col != NULL ){
    int nElements = col->getNumberOfElements()  ;

    for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
      SimCalorimeterHit* hit = dynamic_cast<SimCalorimeterHit*>( col->getElementAt(hitIndex) );
      double currentEnergy = hit->getEnergy();
      double currentPosX = hit->getPosition()[0];
      double currentPosY = hit->getPosition()[1];
      double currentPosZ = hit->getPosition()[2];
      currentPosX = currentPosX - std::abs(hit->getPosition()[2] * 0.007);
     
      if (hit->getPosition()[2] > 0){
	//Positions
	pX = currentPosX;
	pY = currentPosY;

	//total  energy deposit
	ptotalEnergy += currentEnergy;
	
	//LR
	pnum_LR += currentPosX * currentEnergy;
	pdenom_LR += std::abs(currentPosX) * currentEnergy;

	//TD
	pnum_TD += currentPosY * currentEnergy;
	pdenom_TD += std::abs(currentPosY) * currentEnergy;

	//Mean Depth
	pnum_meanDepth = currentPosZ * currentEnergy;

	//r-moment
	prad = std::sqrt((std::pow(currentPosX - barycenters[0],2) + (std::pow(hit->getPosition()[1] - barycenters[1],2))));
	pnum_moment += prad * currentEnergy;
	pdenom_moment += currentEnergy;

	//invr-moment
	pnum_invrmoment += (1 / prad) * currentEnergy;
      }else {
	//Positions                                                                                                                        
	nX = currentPosX;
	nY = currentPosY;

	//total energy deposit
	ntotalEnergy += currentEnergy;

	//LR                                                                                                                               
        nnum_LR+= currentPosX * currentEnergy;
	ndenom_LR += std::abs(currentPosX) * currentEnergy;

	//TD                                                                                                                               
	nnum_TD+= currentPosY * currentEnergy;
	ndenom_TD += std::abs(currentPosY) * currentEnergy;

	//Mean Depth                                                                                                                       
	nnum_meanDepth = currentPosZ * currentEnergy;

	//r-moment
	nrad = std::sqrt((std::pow(currentPosX - barycenters[2],2) + (std::pow(hit->getPosition()[1] - barycenters[3],2))));
	nnum_moment += nrad * currentEnergy;
        ndenom_moment += currentEnergy;

	//invr-moment                                                                                                                      
	nnum_invrmoment += (1 / prad) * currentEnergy;
      }


    }
  }

  double* obs = new double[16];

  //Energy Deposit
  obs[0] = ptotalEnergy;
  obs[1] = ntotalEnergy;
  
  //LR
  pLR = pnum_LR / pdenom_LR;
  nLR = nnum_LR / ndenom_LR;
  obs[2] = pLR;
  obs[3] = nLR;

  //TD
  pTD = pnum_TD / pdenom_TD;
  nTD = nnum_TD / ndenom_TD;
  obs[4] = pTD;
  obs[5] = nTD;

  //mean Depth
  pmeanDepth = pnum_meanDepth / ptotalEnergy;
  nmeanDepth = nnum_meanDepth / ntotalEnergy;
  obs[6] = pmeanDepth;
  obs[7] = nmeanDepth;

  //r-moment
  prmoment = pnum_moment / pdenom_moment;
  nrmoment = nnum_moment / ndenom_moment;
  obs[8] = prmoment;
  obs[9] = nrmoment;

  //invr-moment
  pinvrmoment = pnum_invrmoment / ptotalEnergy;
  ninvrmoment = nnum_invrmoment / ntotalEnergy;
  obs[10] = pinvrmoment;
  obs[11] = ninvrmoment;

  //Positons
  obs[12] = pX;
  obs[13] = pY;    
  obs[14] = nX;
  obs[15] = nY;

  return obs;
  
}

double*  ElliotsAnalysis::calculateBarycenter( LCCollection* col ){
  
  double pbarycenterPosX = 0, pbarycenterPosY = 0, nbarycenterPosX = 0, nbarycenterPosY = 0;
  double pnumX = 0, pnumY = 0, pdenomX = 0, pdenomY = 0, nnumX = 0, nnumY = 0, ndenomX = 0, ndenomY = 0;
  double pEnergy = 0, nEnergy = 0;
    
    if( col != NULL ){
        int nElements = col->getNumberOfElements()  ;

        for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
            SimCalorimeterHit* hit = dynamic_cast<SimCalorimeterHit*>( col->getElementAt(hitIndex) );
	    double currentEnergy = hit->getEnergy();
	    double currentPosX = hit->getPosition()[0];
	    double currentPosY = hit->getPosition()[1];
	    double currentPosZ = hit->getPosition()[2];

	    currentPosX = currentPosX - std::abs(currentPosZ * 0.007);

	    if (currentPosZ < 0){
	      //calculate numerator and denominator of barycenter x value                                                                              
	      nnumX += currentPosX * currentEnergy;
	      ndenomX += currentEnergy;

	      //calculate numerator and denominator of barycenter y value                                                                              
	      nnumY += currentPosY * currentEnergy;
	      ndenomY += currentEnergy;
	    }
	    else {

	      //calculate numerator and denominator of barycenter x value
	      pnumX += currentPosX * currentEnergy;
	      pdenomX += currentEnergy;

	      //calculate numerator and denominator of barycenter y value
	      pnumY += currentPosY * currentEnergy;
	      pdenomY += currentEnergy;
	    }
	 }
    }
    
    pEnergy = pdenomX;
    pbarycenterPosX = pnumX / pdenomX;
    pbarycenterPosY = pnumY / pdenomY;

    nEnergy = ndenomX;
    nbarycenterPosX = nnumX / ndenomX;
    nbarycenterPosY = nnumY / ndenomY;

    double* barycenters = new double[4]; 
    barycenters[0] = pbarycenterPosX;
    barycenters[1] = pbarycenterPosY;
    barycenters[2] = nbarycenterPosX;
    barycenters[3] = nbarycenterPosY;
    
    //    printf("\n\nPositive Barycenter Position: (%f, %f) with Energy: %f\n\n", barycenters[0],barycenters[1], pEnergy);
    // printf("\n\nNegative Barycenter Position: (%f, %f) with Energy: %f\n\n", barycenters[2],barycenters[3], nEnergy);
    
    return barycenters;
} 

void ElliotsAnalysis::printParticleProperties(SimCalorimeterHit* hit){

  
    int type = 0;
    double energy = 0;
    float charge = 0;
    float px = 0, py = 0, pz = 0;

    MCParticle* currentParticle; 
    MCParticle* highestEnergyParticle = hit->getParticleCont(0);
    
    

    for (int i = 0; i < hit->getNMCContributions(); i++){
      currentParticle = hit->getParticleCont(i);
      

      if (currentParticle->getEnergy() > highestEnergyParticle->getEnergy()){
        highestEnergyParticle = currentParticle;
      }

    }
    energy = highestEnergyParticle->getEnergy();
    type =  highestEnergyParticle->getPDG(); 
    px =  highestEnergyParticle->getMomentum()[0];
    py =  highestEnergyParticle->getMomentum()[1];
    pz =  highestEnergyParticle->getMomentum()[2];
    charge =  highestEnergyParticle->getCharge();
    

    printf("\nHighest energy Particle in hit: %0.5f\n", energy);
    printf("Type: %d\n",type);
    printf("Momentum: (%0.2f,%0.2f, %0.2f)\n", px,py,pz);
    printf("Charge: %0.2f\n", charge);
  
 
}

double ElliotsAnalysis::findAvgObs(double* obs){
  double sum = 0;
  double avgObs = 0;

  int num = 0;

  for (int i = 0;i < numEvents ; i++ ){

    sum += obs[i];

    num++;

  }

  avgObs = sum / num;

  return avgObs;

}



void ElliotsAnalysis::processEvent( LCEvent * evt ) { 
    // this gets called for every event 
    // usually the working horse ...

    LCCollection* col = evt->getCollection( _colName ) ;
    
    double* barycenters = calculateBarycenter(col);
    double* obs = calculateObservables(col, barycenters);
    
   
    peventBarycenterX[currentEvent] = barycenters[0];
    peventBarycenterY[currentEvent] = barycenters[1];
    neventBarycenterX[currentEvent] = barycenters[2];
    neventBarycenterY[currentEvent] = barycenters[3];
    pEnergyDep[currentEvent] = obs[0];
    nEnergyDep[currentEvent] = obs[1];
    pLR[currentEvent] = obs[2];
    nLR[currentEvent] = obs[3];
    pTD[currentEvent] = obs[4];
    nTD[currentEvent] = obs[5];
    pmeanDepth[currentEvent] = obs[6];
    nmeanDepth[currentEvent] = obs[7];
    prmoment[currentEvent] = obs[8];
    nrmoment[currentEvent] = obs[9];
    pinvrmoment[currentEvent] = obs[10];
    ninvrmoment[currentEvent] = obs[11];
    pX[currentEvent] = obs[12];
    pY[currentEvent] = obs[13];
    nX[currentEvent] = obs[14];
    nY[currentEvent] = obs[15];

    printf("\n=====================EVENT %d========================== \n", currentEvent + 1);  
    
    printf("\nBARYCENTER Postive:( %f,%f) Negative (%f,%f)", barycenters[0], barycenters[1], barycenters[2], barycenters[3]);
    printf("ENERYG DEPOSIT Postive: %f  Negative: %f", obs[0], obs[1]);
   
  
    
    double highestEnergy = 0;
    double lowestEnergy = 10000;
    double hParticleEnergy = 0;
    double lParticleEnergy = 0;

    float hPosX = 0, hPosY = 0, hPosZ = 0;
    float lPosX = 0, lPosY= 0, lPosZ = 0;

    SimCalorimeterHit* maxHit = dynamic_cast<SimCalorimeterHit*>( col->getElementAt(0));
    SimCalorimeterHit* minHit = dynamic_cast<SimCalorimeterHit*>( col->getElementAt(0));

   
    // this will only be entered if the collection is available
    if( col != NULL ){
        int nElements = col->getNumberOfElements()  ;
	
        for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
           SimCalorimeterHit* hit = dynamic_cast<SimCalorimeterHit*>( col->getElementAt(hitIndex) );
	   
	   
	   //find hit with highest energy
	   if (hit->getEnergy() > highestEnergy){
	     highestEnergy = hit->getEnergy();
	     maxHit = hit;
	     hPosX = hit->getPosition()[0];
	     hPosY = hit->getPosition()[1];
	     hPosZ = hit->getPosition()[2];
	   }

	   //find hit with lowest energy
	   if (hit->getEnergy() < lowestEnergy){
             lowestEnergy = hit->getEnergy();
	     minHit = hit;
             lPosX = hit->getPosition()[0];
             lPosY = hit->getPosition()[1];
             lPosZ = hit->getPosition()[2];
           }

	   

           const float* pos = hit->getPosition();
           _hitmap->Fill(pos[0],pos[1]);
        } 
    }
   
    // printParticleProperties(maxHit);
  
    currentEvent++;
    if (currentEvent % numEvents == 0){

      
      double pAvgBarycenterX = findAvgObs(peventBarycenterX);
      double pAvgBarycenterY = findAvgObs(peventBarycenterY);
      double nAvgBarycenterX = findAvgObs(neventBarycenterX);
      double nAvgBarycenterY = findAvgObs(neventBarycenterY);
      double pAvgEnergyDep = findAvgObs(pEnergyDep);
      double nAvgEnergyDep = findAvgObs(nEnergyDep);
      double pAvgLR = findAvgObs(pLR);
      double nAvgLR = findAvgObs(nLR);
      double pAvgTD = findAvgObs(pTD);
      double nAvgTD = findAvgObs(nTD);
      double pAvgmeanDepth = findAvgObs(pmeanDepth);
      double nAvgmeanDepth = findAvgObs(nmeanDepth);
      double pAvgrmoment = findAvgObs(prmoment);
      double nAvgrmoment = findAvgObs(nrmoment);
      double pAvginvrmoment = findAvgObs(pinvrmoment);
      double nAvginvrmoment = findAvgObs(ninvrmoment);
      double pAvgX = findAvgObs(pX);
      double pAvgY = findAvgObs(pY);
      double nAvgX = findAvgObs(nX);
      double nAvgY = findAvgObs(nY);

      printf("\nAVERAGE BARYCENTER: Postive: (%f,%f) Negative: (%f,%f)", pAvgBarycenterX, pAvgBarycenterY,nAvgBarycenterX, nAvgBarycenterY);
      printf("\nAVERAGE Energy Deposit: Postive: %f Negative: %f", pAvgEnergyDep, nAvgEnergyDep);

      printf("\nAVERAGE Positions: Postive: (%f,%f) Negative: (%f,%f)", pAvgX, pAvgY, nAvgX, nAvgY);

      printf("\nAVERAGE Mean Depth: Postive: %f Negative: %f", pAvgmeanDepth, nAvgmeanDepth);

      printf("\nAVERAGE LR: Postive: %f Negative: %f", pAvgLR, nAvgLR);

      printf("\nAVERAGE TD: Postive: %f Negative: %f", pAvgTD, nAvgTD);

      std::fill_n(peventBarycenterX, numEvents, 0);
      std::fill_n(peventBarycenterY, numEvents, 0);
      std::fill_n(neventBarycenterX, numEvents, 0);
      std::fill_n(neventBarycenterY, numEvents, 0);
      std::fill_n(pEnergyDep, numEvents, 0);
      std::fill_n(nEnergyDep, numEvents, 0);
      std::fill_n(pLR, numEvents, 0);
      std::fill_n(nLR, numEvents, 0);
      std::fill_n(pTD, numEvents, 0);
      std::fill_n(nTD, numEvents, 0);
      std::fill_n(pmeanDepth, numEvents, 0);
      std::fill_n(nmeanDepth, numEvents, 0);
      std::fill_n(prmoment, numEvents, 0);
      std::fill_n(nrmoment, numEvents, 0);
      std::fill_n(pinvrmoment, numEvents, 0);
      std::fill_n(ninvrmoment, numEvents, 0);
      std::fill_n(pX, numEvents, 0);
      std::fill_n(pY, numEvents, 0);
      std::fill_n(nX, numEvents, 0);
      std::fill_n(nY, numEvents, 0);
  }
    

    _nEvt ++ ;
}



void ElliotsAnalysis::check( LCEvent * evt ) { 
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}



void ElliotsAnalysis::end(){


 
    _rootfile->Write();
}
