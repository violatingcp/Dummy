import FWCore.ParameterSet.Config as cms

process = cms.Process('TestPuppi')
process.load('Configuration/StandardSequences/Services_cff')
process.load('FWCore/MessageService/MessageLogger_cfi')
process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')
process.MessageLogger.cerr.FwkReport.reportEvery = 10
process.GlobalTag.globaltag = 'START53_V7G::All'

process.load('Dummy/Puppi/Puppi_cff')   

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(50) )
process.source = cms.Source("PoolSource",
  fileNames  = cms.untracked.vstring('file:/tmp/pharris/RSGravitonToWW_kMpl01_M_3000_Tune4C_13TeV_pythia8_PU_S14_PAT.root')
  #fileNames  = cms.untracked.vstring('/store/relval/CMSSW_7_1_0_pre5/RelValTTbar_13/GEN-SIM-RECO/PU50ns_POSTLS171_V2-v2/00000/4CCC03AC-BDBC-E311-8597-02163E00EA7F.root')
)
process.source.inputCommands = cms.untracked.vstring("keep *",
                                                     "drop *_MEtoEDMConverter_*_*")

process.options = cms.untracked.PSet(
  wantSummary = cms.untracked.bool(True),
  Rethrow     = cms.untracked.vstring('ProductNotFound'),
  fileMode    = cms.untracked.string('NOMERGE')
)


process.puppiSequence = cms.Sequence(process.puppi)
process.p = cms.Path(process.puppiSequence)
process.output = cms.OutputModule("PoolOutputModule",                                                                                                                                                     
                                  outputCommands = cms.untracked.vstring('drop *','keep *_*_*_RECO','drop *_*_Cleaned_*','keep *_puppi_*_*'),
                                  fileName       = cms.untracked.string ("Output.root")                                                                                                                   
)
# schedule definition                                                                                                       
process.outpath  = cms.EndPath(process.output) 
