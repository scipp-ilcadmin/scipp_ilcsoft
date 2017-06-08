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

#include "parser.h"
#include "scipp_ilc_utilities.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/MCParticle.h>

#include <TFile.h>
#include <TH2D.h>

#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <algorithm>
// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"


using namespace lcio;
using namespace marlin;
using namespace std;


parser parser;

static TFile* _rootfile;
static int nBhabha=0;
static int nBase=0;
static int nTwoPhoton=0;
static int nCombo=0;


parser::parser() : Processor("parser") {
    // modify processor description
    _description = "Protype Processor" ;

    // register steering parameters: name, description, class-variable, default value
    registerInputCollection( LCIO::MCPARTICLE, "CollectionName" , "Name of the MCParticle collection"  , _colName , std::string("MCParticle") );
    
    registerProcessorParameter( "RootOutputName" , "output file"  , _root_file_name , std::string("output.root") );
}


void parser::init() { 
    streamlog_out(DEBUG) << "   init called  " << std::endl ;
    _rootfile = new TFile("parser.root","RECREATE");
    _nEvt = 0 ;
}

void parser::processRunHeader( LCRunHeader* run) { 
//    _nRun++ ;
} 

//Returns number of bhabha events.
int parser::countBhabhas(LCCollection* col, PTree* trees){
  //It says there are 30 items in trees but only loops through one...
  //Probably something wrong with this pointer syntax.
  bool v=true; //Verbose Mode
  int bhabhas=0;
  if(v)cout << "###### Trees: " << trees->size() << " ######" << endl;
  for(const auto tree: *trees){
    //    if(v)cout << "This Tree Size: " << tree->size() << endl;    
    bool bhabha=true;
    for(const auto hit: *tree){
      //      if(v)cout << "id: " << hit->getPDG() << " and address " << hit << endl;
      int id = hit->getPDG();
      if ( id != 11 && id != -11 && id != 22){
	bhabha=false;
      }
    }
    if(bhabha){
      //      if(v)cout << "This tree had only bhabha particles  particles." << endl;
      ++bhabhas;
    }
  }
  //  if(v) cout << "There are " << bhabhas << " many bhabhas." << endl;
  return bhabhas;
}

void parser::processEvent( LCEvent * evt ) { 
    LCCollection* col = evt->getCollection( _colName );
    int stat, id =0;
    if( col != NULL ){
        int nElements = col->getNumberOfElements();
	PTree* trees = nTrees(evt);
	cout << "###### Elements: " << nElements << " ######" << endl;
	int bhabhas = countBhabhas(col,trees);
	if(nElements == 4) ++nBase;
	else if(bhabhas > 0){
	  int offset = trees->size()-bhabhas;
	  if(offset == 1) ++nBhabha;
	  else  ++nCombo;
	}else ++nTwoPhoton;
    }
}


void parser::check( LCEvent * evt ) { 
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}



void parser::end(){ 
  cout << "Number of empty events: " << nBase << endl;
  cout << "Number of Bhabha events: " << nBhabha << endl;
  cout << "Number of Two Photon events: " << nTwoPhoton << endl;
  cout << "Number of Two Photon & Bhabha events: " << nCombo << endl;
  _rootfile->Write();
}


bool compareMomentum(const double* A,const double* B){
  return A[0]==B[0]&&A[1]==B[1]&&A[2]==B[2];
}

bool compareVectors(vector<MCParticle *> A, vector<MCParticle*> B){
  bool same = A.size() == B.size();
  if(same){
    for(unsigned int i=0; i < A.size(); ++i){
      if(A.at(i)->getPDG() != B.at(i)->getPDG() || !compareMomentum(A.at(i)->getMomentum(), B.at(i)->getMomentum())){
	return false;
      }
    }
  }
  return true;
}

bool containsParticle(MCParticle* obj, vector<MCParticle*>* list){
  for(auto hit: *list){
    if(hit->getPDG() == obj->getPDG()&&
       compareMomentum(hit->getMomentum(),obj->getMomentum())&&
       hit->getGeneratorStatus() == obj->getGeneratorStatus()&&
       compareVectors(hit->getParents(), obj->getParents())&&
       compareVectors(hit->getDaughters(), obj->getDaughters())){
      return true;
    }else{
      return false;
    }
  }
  return false;
}
bool containsParticles(vector<MCParticle*>*base, vector<MCParticle*>* list){
  for(auto hit: *base){
    if(!containsParticle(hit, list)){
      return false;
    }
  }
  return true;
}
bool isFamily(MCParticle* base, vector<MCParticle*>*list){
  for(auto kin: *list){
    vector<MCParticle*> daughters=kin->getDaughters();
    vector<MCParticle*> parents=kin->getParents();
    vector<MCParticle*>* potentialKin= new vector<MCParticle*>;
    potentialKin->insert(potentialKin->end(), daughters.begin(), daughters.end());
    potentialKin->insert(potentialKin->end(), parents.begin(), parents.end());
    potentialKin->push_back(kin);
    if(containsParticle(base, potentialKin)){
      return true;
    }
  }
  return false;
}
bool isRelated(vector<MCParticle*>* A, vector<MCParticle*>* B){
  for(auto a:*A){
    if(isFamily(a, B)){
      return true;
    }
  }
  return false;
}
double getMomentumValue(double* input){
  return input[0]*input[1]*input[2];
}
bool compareTree(MCParticle* A,MCParticle*B){
  return A.getPGP*A.getEnergy()*A;
}
bool sameTree(vector<MCParticle*> A, vector<MCParticle*>B){
  sort(A.begin(), A.end(), compareTree(A,B));
  sort(B.begin(), B.end(), compareTree(A,B));
  return A==B;
}
void parser::addToTree(vector<MCParticle*>* associate, parser::PTree* trees, vector<MCParticle*>*all){
  if(!containsParticles(associate, all)){
    for(auto tree: *trees){
      if(isRelated(associate, tree)){
	tree->insert(tree->end(),associate->begin(),associate->end());
	return;
      }
    }
    trees->push_back(associate);
    all->insert(all->end(),associate->begin(),associate->end());
  }
}
parser::PTree* removeDuplicates(parser::PTree* input){
  parser::PTree* output=new parser::PTree;
  for (auto tree: *input){
    bool add=true;
    for (auto amp: *output)if(sameTree(amp,tree))add=false;
    if(add)output.push_back(biggestTree(tree, amp));
  }
}


vector<MCParticle*>* traverseDirection(MCParticle* particle, bool up=NULL,bool down=NULL){
  vector<MCParticle*>* output=new vector<MCParticle*>;
  output->push_back(particle);
  vector<MCParticle*> direction;
  if (up!=NULL && up)direction=particle->getParents();
  else if(down!=NULL && down)direction=particle->getDaughters();
  for(auto parent: direction){
    auto element=traverseDirection(hit, up=up, down=down);
    output = output.insert(output.end(),element.begin(),elements.end());
  }
  return output;
}
vector<MCPartile*>* traverse(MCParticle* particle){
  auto parents = traverseDirection(particle, up=true);
  auto children= traverseChildren(particle, down=true);
  return parents->insert(parents->end(), children->begin(), children->end());
}

bool biggestTree(vector<MCParticle*>tree,vector<MCParticle*>amp){
  if(tree.size()>amp.size())return tree;
  else return amp;
}


parser::PTree* parser::nTrees(LCEvent *evt, bool v){
  parser::PTree* trees = new parser::PTree;
  int numTrees = 0;
  LCCollection* col = evt->getCollection( _colName ) ;
  int id, stat;
  if( col != NULL ){
    int nElements = col->getNumberOfElements();
    for(int hitIndex = 0; hitIndex < nElements ; hitIndex++){
      MCParticle* hit = dynamic_cast<MCParticle*>( col->getElementAt(hitIndex));
      if (hit->getGeneratorStatus()!=1) continue;
      trees->push_back(traverse(hit));
    }
    //Each particle now has a tree.
    removeDuplicates(trees);
    return trees;
  }
  return trees;
}

/*

  if(!containsParticle(obj, all) && !containsParticle(obj,all)){
    //Not in tree
    all->push_back(obj);
    all->push_back(associate);
    vector<MCParticle*>* arr = new vector<MCParticle*>;
    arr->push_back(obj);
    arr->push_back(associate);
    trees=push_back(arr);
  }else{
    for(auto tree: *trees){
      bool i = containsParticle(obj, tree);
      bool j = containsParticle(associate, tree);
      if(i && j){
	return;
      }else if(i){
	all->push_back(obj);
	tree->push_back(obj);
      }else{
	all->push_back(associate);
	tree->push_back(associate);
      }
    }
  }
*/