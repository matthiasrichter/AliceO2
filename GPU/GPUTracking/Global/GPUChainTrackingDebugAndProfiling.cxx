// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GPUChainTrackingDebugAndProfiling.cxx
/// \author David Rohr

#include "GPUChainTracking.h"
#ifdef GPUCA_TRACKLET_CONSTRUCTOR_DO_PROFILE
#include "bitmapfile.h"
#include <memory>
#endif
#define PROFILE_MAX_SIZE (100 * 1024 * 1024)

using namespace GPUCA_NAMESPACE::gpu;

static inline unsigned int RGB(unsigned char r, unsigned char g, unsigned char b) { return (unsigned int)r | ((unsigned int)g << 8) | ((unsigned int)b << 16); }

int GPUChainTracking::PrepareProfile()
{
#ifdef GPUCA_TRACKLET_CONSTRUCTOR_DO_PROFILE
  char* tmpMem = (char*)mRec->AllocateUnmanagedMemory(PROFILE_MAX_SIZE, GPUMemoryResource::MEMORY_GPU);
  processorsShadow()->tpcTrackers[0].mStageAtSync = tmpMem;
  runKernel<GPUMemClean16>({BlockCount(), ThreadCount(), -1}, nullptr, krnlRunRangeNone, krnlEventNone, tmpMem, PROFILE_MAX_SIZE);
#endif
  return 0;
}

int GPUChainTracking::DoProfile()
{
#ifdef GPUCA_TRACKLET_CONSTRUCTOR_DO_PROFILE
  std::unique_ptr<char[]> stageAtSync{new char[PROFILE_MAX_SIZE]};
  mRec->GPUMemCpy(stageAtSync.get(), processorsShadow()->tpcTrackers[0].mStageAtSync, PROFILE_MAX_SIZE, -1, false);

  FILE* fp = fopen("profile.txt", "w+");
  FILE* fp2 = fopen("profile.bmp", "w+b");

  const int bmpheight = 8192;
  BITMAPFILEHEADER bmpFH;
  BITMAPINFOHEADER bmpIH;
  memset(&bmpFH, 0, sizeof(bmpFH));
  memset(&bmpIH, 0, sizeof(bmpIH));

  bmpFH.bfType = 19778; //"BM"
  bmpFH.bfSize = sizeof(bmpFH) + sizeof(bmpIH) + (ConstructorBlockCount() * ConstructorThreadCount() / 32 * 33 - 1) * bmpheight;
  bmpFH.bfOffBits = sizeof(bmpFH) + sizeof(bmpIH);

  bmpIH.biSize = sizeof(bmpIH);
  bmpIH.biWidth = ConstructorBlockCount() * ConstructorThreadCount() / 32 * 33 - 1;
  bmpIH.biHeight = bmpheight;
  bmpIH.biPlanes = 1;
  bmpIH.biBitCount = 32;

  fwrite(&bmpFH, 1, sizeof(bmpFH), fp2);
  fwrite(&bmpIH, 1, sizeof(bmpIH), fp2);

  int nEmptySync = 0;
  for (unsigned int i = 0; i < bmpheight * ConstructorBlockCount() * ConstructorThreadCount(); i += ConstructorBlockCount() * ConstructorThreadCount()) {
    int fEmpty = 1;
    for (unsigned int j = 0; j < ConstructorBlockCount() * ConstructorThreadCount(); j++) {
      fprintf(fp, "%d\t", stageAtSync[i + j]);
      int color = 0;
      if (stageAtSync[i + j] == 1) {
        color = RGB(255, 0, 0);
      }
      if (stageAtSync[i + j] == 2) {
        color = RGB(0, 255, 0);
      }
      if (stageAtSync[i + j] == 3) {
        color = RGB(0, 0, 255);
      }
      if (stageAtSync[i + j] == 4) {
        color = RGB(255, 255, 0);
      }
      fwrite(&color, 1, sizeof(int), fp2);
      if (j > 0 && j % 32 == 0) {
        color = RGB(255, 255, 255);
        fwrite(&color, 1, 4, fp2);
      }
      if (stageAtSync[i + j]) {
        fEmpty = 0;
      }
    }
    fprintf(fp, "\n");
    if (fEmpty) {
      nEmptySync++;
    } else {
      nEmptySync = 0;
    }
    (void)nEmptySync;
    // if (nEmptySync == GPUCA_SCHED_ROW_STEP + 2) break;
  }

  fclose(fp);
  fclose(fp2);
#endif
  return 0;
}

void GPUChainTracking::PrintMemoryStatistics()
{
  unsigned int nStartHits = 0, nMaxStartHits = 0;
  unsigned int nTracklets = 0, nMaxTracklets = 0;
  unsigned int nTrackletHits = 0, nMaxTrackletHits = 0;
  unsigned int nSectorTracks = 0, nMaxSectorTracks = 0;
  unsigned int nSectorTrackHits = 0, nMaxSectorTrackHits = 0;
  float maxUsageHits = 0.f, maxUsageTracklets = 0.f, maxUsageTrackletHits = 0.f, maxUsageTracks = 0.f, maxUsageTrackHits = 0.f;
  for (int i = 0; i < NSLICES; i++) {
    nMaxStartHits += processors()->tpcTrackers[i].NMaxStartHits();
    nStartHits += *processors()->tpcTrackers[i].NStartHits();
    maxUsageHits = CAMath::Max(maxUsageHits, 100.f * *processors()->tpcTrackers[i].NStartHits() / std::max(1u, processors()->tpcTrackers[i].NMaxStartHits()));
    nMaxTracklets += processors()->tpcTrackers[i].NMaxTracklets();
    nTracklets += *processors()->tpcTrackers[i].NTracklets();
    maxUsageTracklets = CAMath::Max(maxUsageTracklets, 100.f * *processors()->tpcTrackers[i].NTracklets() / std::max(1u, processors()->tpcTrackers[i].NMaxTracklets()));
    nMaxTrackletHits += processors()->tpcTrackers[i].NMaxRowHits();
    nTrackletHits += *processors()->tpcTrackers[i].NRowHits();
    maxUsageTrackletHits = CAMath::Max(maxUsageTrackletHits, 100.f * *processors()->tpcTrackers[i].NRowHits() / std::max(1u, processors()->tpcTrackers[i].NMaxRowHits()));
    nMaxSectorTracks += processors()->tpcTrackers[i].NMaxTracks();
    nSectorTracks += *processors()->tpcTrackers[i].NTracks();
    maxUsageTracks = CAMath::Max(maxUsageTracks, 100.f * *processors()->tpcTrackers[i].NTracks() / std::max(1u, processors()->tpcTrackers[i].NMaxTracks()));
    nMaxSectorTrackHits += processors()->tpcTrackers[i].NMaxTrackHits();
    nSectorTrackHits += *processors()->tpcTrackers[i].NTrackHits();
    maxUsageTrackHits = CAMath::Max(maxUsageTrackHits, 100.f * *processors()->tpcTrackers[i].NTrackHits() / std::max(1u, processors()->tpcTrackers[i].NMaxTrackHits()));
  }
  unsigned int nTracks = processors()->tpcMerger.NOutputTracks();
  unsigned int nMaxTracks = processors()->tpcMerger.NMaxTracks();
  unsigned int nTrackHits = processors()->tpcMerger.NOutputTrackClusters();
  unsigned int nMaxTrackHits = processors()->tpcMerger.NMaxOutputTrackClusters();

  GPUInfo("Mem Usage Start Hits     : %8u / %8u (%3.0f%% / %3.0f%%)", nStartHits, nMaxStartHits, 100.f * nStartHits / std::max(1u, nMaxStartHits), maxUsageHits);
  GPUInfo("Mem Usage Tracklets      : %8u / %8u (%3.0f%% / %3.0f%%)", nTracklets, nMaxTracklets, 100.f * nTracklets / std::max(1u, nMaxTracklets), maxUsageTracklets);
  GPUInfo("Mem Usage TrackletHits   : %8u / %8u (%3.0f%% / %3.0f%%)", nTrackletHits, nMaxTrackletHits, 100.f * nTrackletHits / std::max(1u, nMaxTrackletHits), maxUsageTrackletHits);
  GPUInfo("Mem Usage SectorTracks   : %8u / %8u (%3.0f%% / %3.0f%%)", nSectorTracks, nMaxSectorTracks, 100.f * nSectorTracks / std::max(1u, nMaxSectorTracks), maxUsageTracks);
  GPUInfo("Mem Usage SectorTrackHits: %8u / %8u (%3.0f%% / %3.0f%%)", nSectorTrackHits, nMaxSectorTrackHits, 100.f * nSectorTrackHits / std::max(1u, nMaxSectorTrackHits), maxUsageTrackHits);
  GPUInfo("Mem Usage Tracks         : %8u / %8u (%3.0f%%)", nTracks, nMaxTracks, 100.f * nTracks / nMaxTracks);
  GPUInfo("Mem Usage TrackHits      : %8u / %8u (%3.0f%%)", nTrackHits, nMaxTrackHits, 100.f * nTrackHits / nMaxTrackHits);
}

void GPUChainTracking::PrintMemoryRelations()
{
  for (int i = 0; i < NSLICES; i++) {
    GPUInfo("MEMREL StartHits NCl %d NTrkl %d", processors()->tpcTrackers[i].NHitsTotal(), *processors()->tpcTrackers[i].NStartHits());
    GPUInfo("MEMREL Tracklets NCl %d NTrkl %d", processors()->tpcTrackers[i].NHitsTotal(), *processors()->tpcTrackers[i].NTracklets());
    GPUInfo("MEMREL Tracklets NCl %d NTrkl %d", processors()->tpcTrackers[i].NHitsTotal(), *processors()->tpcTrackers[i].NRowHits());
    GPUInfo("MEMREL SectorTracks NCl %d NTrk %d", processors()->tpcTrackers[i].NHitsTotal(), *processors()->tpcTrackers[i].NTracks());
    GPUInfo("MEMREL SectorTrackHits NCl %d NTrkH %d", processors()->tpcTrackers[i].NHitsTotal(), *processors()->tpcTrackers[i].NTrackHits());
  }
  GPUInfo("MEMREL Tracks NCl %d NTrk %d", processors()->tpcMerger.NMaxClusters(), processors()->tpcMerger.NOutputTracks());
  GPUInfo("MEMREL TrackHitss NCl %d NTrkH %d", processors()->tpcMerger.NMaxClusters(), processors()->tpcMerger.NOutputTrackClusters());
}

void GPUChainTracking::PrepareDebugOutput()
{
#ifdef GPUCA_KERNEL_DEBUGGER_OUTPUT
  const auto& threadContext = GetThreadContext();
  if (mRec->IsGPU()) {
    SetupGPUProcessor(&processors()->debugOutput, false);
    WriteToConstantMemory(RecoStep::NoRecoStep, (char*)&processors()->debugOutput - (char*)processors(), &processorsShadow()->debugOutput, sizeof(processors()->debugOutput), -1);
    memset(processors()->debugOutput.memory(), 0, processors()->debugOutput.memorySize() * sizeof(processors()->debugOutput.memory()[0]));
  }
  runKernel<GPUMemClean16>({BlockCount(), ThreadCount(), 0, RecoStep::TPCSliceTracking}, krnlRunRangeNone, {}, (mRec->IsGPU() ? processorsShadow() : processors())->debugOutput.memory(), processorsShadow()->debugOutput.memorySize() * sizeof(processors()->debugOutput.memory()[0]));
#endif
}

void GPUChainTracking::PrintDebugOutput()
{
#ifdef GPUCA_KERNEL_DEBUGGER_OUTPUT
  const auto& threadContext = GetThreadContext();
  TransferMemoryResourcesToHost(RecoStep::NoRecoStep, &processors()->debugOutput, -1);
  processors()->debugOutput.Print();
#endif
}
