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

//#include <ctime>    //************************************************************
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cmath>

#include "BeamCalRecon_xy.h"
#include "scipp_ilc_utilities.h"
#include "polar_coords.h"
//#include "beamcal_reconstructor.h"
#include "include/beamcal_reconstructor_xy.h"
//#include "beamcal_scanner.h"
#include "include/beamcal_scanner_xy.h"

#include <EVENT/LCCollection.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/MCParticle.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#include <TFile.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TPaveStats.h>

typedef std::chrono::high_resolution_clock Clock;

using namespace lcio;
using namespace marlin;
using namespace std;


BeamCalRecon_xy BeamCalRecon_xy;

static TFile* _rootfile;
static TProfile* _radeff;
static int _detected_num = 0;
static int _test_num = 0;
//static TH2F* _hitmap_bgd;
//static TH2D* _hitmap_bgd;
static TProfile2D* _hitmap_bgd;
static TProfile2D* _hitmap_zeros;
static TProfile2D* _test_slice;

static TH2F* _hlego;
static TH2F* _hlego_zeros;
static TH2F* _hlego_inefficiency;
static TH2F* _hlego_test;

//static TH2F* _hlego_var;
//static TH2F* _hlego_zeros_var;
//static TH2F* _hlego_inefficiency_var;
//static TH2F* _hlego_test_var;

static TCanvas* _c2;

static unordered_map<pair<float,float>,double>* _all_map;
static unordered_map<pair<float,float>,double>* _zeros_map;
//vector<pair<float,float>> bgd_plot_xy;


//auto _t1 = Clock::now();
//auto _t2 = Clock::now();


BeamCalRecon_xy::BeamCalRecon_xy() : Processor("BeamCalRecon_xy") {
    // modify processor description
    _description = "Protype Processor" ;

    // register steering parameters: name, description, class-variable, default value
    registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );
    registerProcessorParameter( "BeamcalGeometryFile" , "input file"  , _beamcal_geometry_file_name , std::string("input.xml") ) ;
    registerProcessorParameter( "BackgroundEventList" , "input file"  , _background_event_list , std::string("input.xml") ) ;
    registerProcessorParameter( "BackgroundEventsToRead" , "number"  , _num_bgd_events_to_read , 10 ) ;
    registerProcessorParameter( "RootOutputName" , "output file"  , _root_file_name , std::string("output.root") );
}


// This function edits a root plot passed from init
void BeamCalRecon_xy::RootPlot(TH2F* graph){

  graph->GetXaxis()->SetTitle("X axis (mm)");
  graph->GetYaxis()->SetTitle("Y axis (mm)");
  graph->GetZaxis()->SetTitle("Efficiency");

  graph->GetXaxis()->CenterTitle();
  graph->GetYaxis()->CenterTitle();
  graph->GetZaxis()->CenterTitle();

  graph->GetXaxis()->SetTitleOffset(1.4);
  graph->GetYaxis()->SetTitleOffset(1.6);

  //  graph->SetPhi(60);
  //  TPaveStats *st = (TPaveStats*)graph->FindObject("stats");
  //  st->SetX1NDC(0.0);
  //  st->SetX2NDC(0.0);
  //    gStyle->SetOptStat(0);
}



void BeamCalRecon_xy::init() { 
    streamlog_out(DEBUG) << "   init called  " << std::endl ;
    //    gStyle->SetStatX(0.1);

    _rootfile = new TFile(_root_file_name.c_str(),"RECREATE");
    _radeff = new TProfile("radeff","Radial Efficiency",14*2,0.0,140.0,0.0,1.0);
    _hitmap_bgd = new TProfile2D("hitmap_bgd","Hit Distribution",300.0,-150.0,150.0,300.0,-150.0,150.0);
    _hitmap_zeros = new TProfile2D("hitmap_zeros","Hit Distribution",300.0,-150.0,150.0,300.0,-150.0,150.0);
    _test_slice = new TProfile2D("hitmap_slice","Hit Distribution",300.0,-150.0,150.0,300.0,-150.0,150.0);
    _c2 = new TCanvas("c2","c2",300,300);

    int LEGObins = 60;
    const Int_t NBINS = 68;
    //    Double_t edges[NBINS + 1] = {};
    //    static float edges[] = { -199.50, -189.57, -181.02, -172.60,
    Double_t edges[69] = { -199.50, -189.57, -181.02, -172.60,
			   -164.32,    -156.17,    -148.16,    -140.30,    -132.57,
			   -125.00,    -117.58,    -110.30,    -103.19,    -96.23,
			   -89.44,    -82.82,    -76.37,    -70.09,    -64.00,
			   -58.09,    -52.38,    -46.87,    -41.57,    -36.48,
			   -31.62,    -27.00,    -22.63,    -18.52,    -14.70,
			   -11.18,    -8.00,    -5.20,    -2.83,    -1.00,
			   0.00,    1.00,    2.83,    5.20,    8.00,
			   11.18,    14.70,    18.52,    22.63,    27.00,
			   31.62,    36.48,    41.57,    46.87,    52.38,
			   58.09,    64.00,    70.09,    76.37,    82.82,
			   89.44,    96.23,    103.19,    110.30,    117.58,
			   125.00,    132.57,    140.30,    148.16,    156.17,
			   164.32, 172.60, 181.02, 189.57, 199.50};
    std::stringstream s1;

    s1 << "LEGO 1s,"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOtitle = s1.str().c_str();
    _hlego = new TH2F("hlego", LEGOtitle ,LEGObins ,-150,150,LEGObins,-150,150);
    RootPlot(_hlego);

    s1.str("");
    s1 << "LEGO 0s,"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOtitlez = s1.str().c_str();
    _hlego_zeros = new TH2F("hlego_0s", LEGOtitlez ,LEGObins,-150,150,LEGObins,-150,150);
    RootPlot(_hlego_zeros);

    s1.str("");
    s1 << "LEGO,"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOtitleInefficiency = s1.str().c_str();
    _hlego_inefficiency = new TH2F("hlego_inefficiency", LEGOtitleInefficiency, LEGObins,-150,150,LEGObins,-150,150);
    //    _hlego_inefficiency_var = new TH2F("hlego_inefficiency_var", LEGOTitleInefficiency, 67, edges, 67, edges);
    RootPlot(_hlego_inefficiency);

    s1.str("");
    s1 << "LEGO , test"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOTestTitle = s1.str().c_str();
    _hlego_test = new TH2F("hlego_test", LEGOTestTitle ,LEGObins,-150,150,LEGObins,-150,150);
    //    _hlego_test_var = new TH2F("hlego_test_var", LEGOTestTitle, 67, edges, 67, edges);
    RootPlot(_hlego_test);

    /*
    s1.str("");
    s1 << "LEGO 1s, var"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOtitleVar = s1.str().c_str();
    _hlego_var = new TH2F("hlego_var", LEGOtitleVar, 67, edges, 67, edges);

    s1.str("");
    s1 << "LEGO 0s, var"<< _num_bgd_events_to_read << "events," << LEGObins << "bin";
    const char* LEGOtitlezVar = s1.str().c_str();
    _hlego_zeros_var = new TH2F("hlego_0s_var", LEGOtitlezVar, 67, edges, 67, edges);

     */


    // TH2F
    // TProfile2D
    //    _t1 = Clock::now();

    //Load up all the bgd events, and initialize the reconstruction algorithm.
    scipp_ilc::beamcal_recon_xy::initialize_beamcal_reconstructor(_beamcal_geometry_file_name, _background_event_list, _num_bgd_events_to_read);

    _nRun = 0 ;
    _nEvt = 0 ;

}



void BeamCalRecon_xy::processRunHeader( LCRunHeader* run) { 
//    _nRun++ ;
} 



void BeamCalRecon_xy::processEvent( LCEvent* signal_event ) {
    //Make sure we are using an electron that actually hits the Positive BeamCal

  //  _hitmap_bgd->Fill

    MCParticle* electron = NULL;
    bool detectable_electron = scipp_ilc::get_detectable_signal_event(signal_event,electron);
    if ( not detectable_electron ) return;

    //Get the radius at which the signal electron hit
    const double* endpoint = electron->getEndpoint();
    double end_x = (endpoint[0] - 0.007*endpoint[2]);
    double end_y = endpoint[1];
    double endx = end_x;
    double endy = end_y;

    double radius,phi;
    scipp_ilc::cartesian_to_polar(end_x,end_y,radius,phi);


    //Perform the reconstrunction algorithm, determine if the algorithm
    //detected the electron.
    scipp_ilc::beamcal_recon_xy::beamcal_cluster* signal_cluster;
    signal_cluster = scipp_ilc::beamcal_recon_xy::reconstruct_beamcal_event(signal_event);
    bool detected = signal_cluster->exceeds_sigma_cut;

    //      cout << "******************************************************************" << endl;
    //      cout << "detected:  " << detected<< "        x-y: "<< endx << "\t" <<endy << endl;
    //      cout << "******************************************************************" << endl;
    
    pair<float,float> pos;
    pos.first = (float) endx;
    pos.second = (float) endy;
	
    string endx_s = std::to_string(pos.first);
    string endy_s = std::to_string(pos.second);
    cout << "endx string"<< endx_s << endl;

    string ID = endx_s + "," + endy_s;
    cout << "ID string"<< ID << endl;


    //	int ID = scipp_ilc::beamcal_recon_xy::getID(end_x,end_y);
    //	int ID = scipp_ilc::simple_list_geometry_xy::getID(end_x,end_y);


    //Plot our results with respect to the radius of the signal electron.
    _radeff->Fill(radius,detected);             //bools and ints are basically interchangeable...
    _detected_num += detected;

    if(detected && endx > 0 && endy < 0){       //Graph of slice of beamcal
      _test_num += detected;
      _test_slice->Fill(endx,endy,detected);
    }
    if(detected){                               //Graph of detected
      _hitmap_bgd->Fill(endx,endy,detected);    //      _hitmap_bgd->Fill(endx,endy);
      _hlego->Fill(endx,endy,detected);
      //      _hlego_var->Fill(endx,endy,detected);
    }else{                                      //Graph of not detected
      _hitmap_zeros->Fill(endx,endy,true);
      _hlego_zeros->Fill(endx,endy,true);
      //      _hlego_zeros_var->Fill(endx,endy,true);
      
      //      if((_zeros_map)[pos]>=1.0){
      //	(_zeros_map)[pos]+= 1.0;
      //      }else{
      //	(_zeros_map)[pos] = 1.0;
      //      }
    }
    //    if(!detected){                              //Print out xy-coords of not detected
    //      cout << "detected:  " << detected<< "        x-y: "<< endx << "\t" <<endy << endl;
    //    }
    //    (*_all_map)[ID]+= (detected || !detected);


    _hlego->SetFillColor(kYellow);
    cout << _nEvt++ << endl;
}


void BeamCalRecon_xy::check( LCEvent * evt ){
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void BeamCalRecon_xy::end(){ 
  //  for(auto bit:*_zeros_map){
  //    pair<float,float> ID = bit.first;
  //    double bit_hit = bit.second;
  //    double all_hit = *_all_map[ID];
  //  _hlego_inefficiency->Fill(ID.first,ID.second,bit_hit/all_hit)
  //  }

  _hlego_inefficiency->Add(_hlego_zeros);
  _hlego_test->Add(_hlego_zeros);
  _hlego_test->Add(_hlego);
  _hlego_inefficiency->Divide(_hlego_test);

    cout << "\ndetected: " << _detected_num << endl;
    cout << "\n in \'slice\' of beamcal: " << _test_num << endl;

    //    _t2 = Clock::now();
    //    cout << "*******************this is the end***********************" << endl;
    //    cout << "******************* time elapsed: " << (_end - _begin) << " ***********************" << endl;
    //    cout << "******************* time elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(_t2 - _t1).count() << " ***********************" << endl;
    _rootfile->Write();
}
