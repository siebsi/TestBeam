/* 
 * Determination of the position resolution of the setup.
 */

/**
	@Author: Thorben Quast <tquast>
		14 September 2017
		thorben.quast@cern.ch / thorben.quast@rwth-aachen.de
*/


// system include files
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <math.h>
// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "HGCal/CondObjects/interface/HGCalCondObjectTextIO.h"
#include "HGCal/DataFormats/interface/HGCalTBWireChamberData.h"
#include "HGCal/DataFormats/interface/HGCalTBDWCTrack.h"


#include "HGCal/Reco/interface/PositionResolutionHelpers.h"
#include "HGCal/Reco/interface/Tracks.h"
#include "HGCal/Reco/interface/Sensors.h"


//#define DEBUG


class DWCTrackProducer : public edm::EDProducer {
	 public:
		DWCTrackProducer(const edm::ParameterSet&);
		virtual void produce(edm::Event&, const edm::EventSetup&);
	 private:
		virtual void beginJob() override;

		edm::EDGetTokenT<WireChambers> MWCToken;
		std::string m_outputTrackName;

		std::map<int, SensorHitMap*> Sensors;
		ParticleTrack* DWCParticleTrack;
};

DWCTrackProducer::DWCTrackProducer(const edm::ParameterSet& iConfig) {	
	
	MWCToken= consumes<WireChambers>(iConfig.getParameter<edm::InputTag>("MWCHAMBERS"));
	m_outputTrackName = iConfig.getParameter<std::string>("OutputCollectionName");

	produces <HGCalTBDWCTrack>(m_outputTrackName);

}//constructor ends here


// ------------ method called for each event  ------------
void DWCTrackProducer::produce(edm::Event& event, const edm::EventSetup& setup) {


	edm::Handle<WireChambers> dwcs;
	event.getByToken(MWCToken, dwcs);

	#ifdef DEBUG
		std::cout<<dwcs->at(0).x<<"  "<<dwcs->at(0).y<<"   "<<dwcs->at(0).z<<"  "<<dwcs->at(0).goodMeasurement<<std::endl;
		std::cout<<dwcs->at(1).x<<"  "<<dwcs->at(1).y<<"   "<<dwcs->at(1).z<<"  "<<dwcs->at(1).goodMeasurement<<std::endl;
		std::cout<<dwcs->at(2).x<<"  "<<dwcs->at(2).y<<"   "<<dwcs->at(2).z<<"  "<<dwcs->at(2).goodMeasurement<<std::endl;
		std::cout<<dwcs->at(3).x<<"  "<<dwcs->at(3).y<<"   "<<dwcs->at(3).z<<"  "<<dwcs->at(3).goodMeasurement<<std::endl;
	#endif


	int NgoodDWCs = 0;
	int DWCReferenceType = 0;
	DWCParticleTrack = new ParticleTrack();	

	for (size_t i=0; i<dwcs->size(); i++) {
		if (!dwcs->at(i).goodMeasurement) continue;
		
		NgoodDWCs++;
		DWCReferenceType += pow(2, i);
		
		Sensors[NgoodDWCs] = new SensorHitMap(NgoodDWCs);				//attention: This is specifically tailored for the 8-layer setup
		Sensors[NgoodDWCs]->setLabZ(dwcs->at(i).z, 0.001);
		Sensors[NgoodDWCs]->setCenterHitPosition(dwcs->at(i).x, dwcs->at(i).y, dwcs->at(i).res_x , dwcs->at(i).res_y);
		Sensors[NgoodDWCs]->setResidualResolution(dwcs->at(i).res_x);	
	
		DWCParticleTrack->addFitPoint(Sensors[NgoodDWCs]);

	}

	std::auto_ptr<HGCalTBDWCTrack> DWCTrack(new HGCalTBDWCTrack);

	if (NgoodDWCs>=2) {
		DWCTrack->valid = true;

		DWCParticleTrack->fitTrack(LINEFITANALYTICAL);
		#ifdef DEBUG
			std::cout<<"DWC reference type: "<<DWCReferenceType<<std::endl;
		#endif

		DWCTrack->b_x = DWCParticleTrack->calculatePositionXY(0., 0).first;
		DWCTrack->b_y = DWCParticleTrack->calculatePositionXY(0., 0).second;
		DWCTrack->m_x = DWCParticleTrack->calculatePositionXY(1., 0).first - DWCTrack->b_x;
		DWCTrack->m_y = DWCParticleTrack->calculatePositionXY(1., 0).second - DWCTrack->b_y;

	    DWCTrack->referenceType = DWCReferenceType;
	    DWCTrack->N_points = NgoodDWCs;
		DWCTrack->chi2_x = DWCParticleTrack->getChi2(1);
		DWCTrack->chi2_y = DWCParticleTrack->getChi2(2);
	    		
	} else {
		DWCTrack->valid = false;
	}
	event.put(DWCTrack, m_outputTrackName);

	for (std::map<int, SensorHitMap*>::iterator it=Sensors.begin(); it!=Sensors.end(); it++) {
		delete (*it).second;
	};	Sensors.clear();
	delete DWCParticleTrack;

}// analyze ends here

void DWCTrackProducer::beginJob() {	
}



//define this as a plug-in
DEFINE_FWK_MODULE(DWCTrackProducer);