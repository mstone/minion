
#ifndef MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#define MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#include "neighbourhood-def.h"
#include <utility>
static const std::string indent = "    ";

struct NeighbourhoodStats {
  DomainInt newMinValue;
  u_int64_t timeTaken;
  bool solutionFound;
  bool timeoutReached;
  DomainInt highestNeighbourhoodSize;

  NeighbourhoodStats(DomainInt newMinValue, u_int64_t timeTaken, bool solutionFound,
                     bool timeoutReached, DomainInt highestNeighbourhoodSize = 0)
      : newMinValue(newMinValue),
        timeTaken(timeTaken),
        solutionFound(solutionFound),
        timeoutReached(timeoutReached),
        highestNeighbourhoodSize(highestNeighbourhoodSize) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodStats& stats) {
    cout << "New Min Value: " << stats.newMinValue << "\n"
         << "Time Taken: " << stats.timeTaken << "\n"
         << "Solution Found: " << stats.solutionFound << "\n"
         << "Timeout Reached: " << stats.timeoutReached << "\n";
    return cout;
  }
};

struct ExplorationPhase {
  int neighbourhoodSize;
  u_int64_t startExplorationTime;
  u_int64_t endExplorationTime;
  int numberOfRandomSolutionsPulled;
};

class NeighbourhoodSearchStats {
public:
  int numberIterations;
  vector<int> numberActivations; // mapping from nh index to number of times activated
  vector<u_int64_t> totalTime;
  vector<int> numberPositiveSolutions;
  vector<int> numberNegativeSolutions;
  vector<int> numberNoSolutions;
  vector<int> numberTimeouts;
  vector<pair<DomainInt, u_int64_t>> bestSolutions;
  int numberOfExplorationPhases;
  int numberOfBetterSolutionsFoundFromExploration;

  vector<int> numberExplorationsByNHSize;
  vector<int> numberSuccessfulExplorationsByNHSize;
  vector<u_int64_t> neighbourhoodExplorationTimes;
  vector<ExplorationPhase> explorationPhases;

  int totalNumberOfRandomSolutionsPulled;
  int numberPulledThisPhase;

  const std::pair<DomainInt, DomainInt> initialOptVarRange;
  DomainInt valueOfInitialSolution;
  DomainInt lastOptVarValue;
  DomainInt bestOptVarValue;
  std::chrono::high_resolution_clock::time_point startTime;
  std::chrono::high_resolution_clock::time_point startExplorationTime;
  bool currentlyExploring = false;
  int currentNeighbourhoodSize;
  u_int64_t totalTimeToBestSolution;
  NeighbourhoodSearchStats() {}

  NeighbourhoodSearchStats(int numberNeighbourhoods,
                           const std::pair<DomainInt, DomainInt>& initialOptVarRange,
                           int maxNeighbourhoodSize)
      : numberIterations(0),
        numberActivations(numberNeighbourhoods, 0),
        totalTime(numberNeighbourhoods, 0),
        numberPositiveSolutions(numberNeighbourhoods, 0),
        numberNegativeSolutions(numberNeighbourhoods, 0),
        numberNoSolutions(numberNeighbourhoods, 0),
        numberTimeouts(numberNeighbourhoods, 0),
        numberOfExplorationPhases(0),
        numberOfBetterSolutionsFoundFromExploration(0),
        initialOptVarRange(initialOptVarRange),
        valueOfInitialSolution(initialOptVarRange.first),
        lastOptVarValue(initialOptVarRange.first),
        numberExplorationsByNHSize(maxNeighbourhoodSize),
        bestOptVarValue(initialOptVarRange.first),
        numberSuccessfulExplorationsByNHSize(maxNeighbourhoodSize),
        neighbourhoodExplorationTimes(maxNeighbourhoodSize),
        totalNumberOfRandomSolutionsPulled(0) {}

  inline u_int64_t getTotalTimeTaken() {
    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
  }

  inline void setValueOfInitialSolution(DomainInt valueOfInitialSolution) {
    this->valueOfInitialSolution = valueOfInitialSolution;
    totalTimeToBestSolution = getTotalTimeTaken();
  }
  inline void startTimer() {
    startTime = std::chrono::high_resolution_clock::now();
  }

  inline void reportnewStats(const vector<int>& activatedNeighbourhoods,
                             const NeighbourhoodStats& stats) {
    ++numberIterations;
    for(int nhIndex : activatedNeighbourhoods) {
      ++numberActivations[nhIndex];
      totalTime[nhIndex] += stats.timeTaken;
      numberTimeouts[nhIndex] += stats.timeoutReached;
      if(stats.solutionFound) {
        if(stats.newMinValue > lastOptVarValue) {
          ++numberPositiveSolutions[nhIndex];
        } else {
          ++numberNegativeSolutions[nhIndex];
        }
        lastOptVarValue = stats.newMinValue;
      } else {
        ++numberNoSolutions[nhIndex];
      }

      if(lastOptVarValue > bestOptVarValue) {
        bestSolutions.emplace_back(lastOptVarValue, getTotalTimeTaken());
        bestOptVarValue = lastOptVarValue;
        totalTimeToBestSolution = getTotalTimeTaken();
      }
    }
  }

  void foundSolution(DomainInt solutionValue) {
    if(currentlyExploring && solutionValue > bestOptVarValue) {
      auto endTime = std::chrono::high_resolution_clock::now();
      neighbourhoodExplorationTimes[currentNeighbourhoodSize - 1] +=
          std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startExplorationTime)
              .count();
      currentlyExploring = false;
      numberSuccessfulExplorationsByNHSize[currentNeighbourhoodSize - 1] += 1;
      explorationPhases.back().endExplorationTime = getTotalTimeTaken();
      explorationPhases.back().numberOfRandomSolutionsPulled = numberPulledThisPhase;
      numberPulledThisPhase = 0;
    }
  }

  void startExploration(int neighbourhoodSize) {
    if(currentlyExploring) {
      auto endTime = std::chrono::high_resolution_clock::now();
      neighbourhoodExplorationTimes[currentNeighbourhoodSize - 1] +=
          std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startExplorationTime)
              .count();
      explorationPhases.back().endExplorationTime = getTotalTimeTaken();
      explorationPhases.back().numberOfRandomSolutionsPulled = numberPulledThisPhase;
      numberPulledThisPhase = 0;
    }
    currentlyExploring = true;
    startExplorationTime = std::chrono::high_resolution_clock::now();
    numberExplorationsByNHSize[neighbourhoodSize - 1] += 1;
    currentNeighbourhoodSize = neighbourhoodSize;
    ExplorationPhase currentPhase;
    currentPhase.neighbourhoodSize = neighbourhoodSize;
    currentPhase.startExplorationTime = getTotalTimeTaken();
    explorationPhases.push_back(currentPhase);
  }

  inline void printStats(std::ostream& os, const NeighbourhoodContainer& nhc) {
    os << "Search Stats:\n";
    os << "Number iterations: " << numberIterations << "\n";
    os << "Initial optimise var range: " << initialOptVarRange << "\n";
    os << "Most recent optimise var value: " << lastOptVarValue << "\n";
    os << "Best optimise var value: " << bestOptVarValue << "\n";
    os << "Time till best solution: " << totalTimeToBestSolution << " (ms)\n";
    os << "Total time: " << getTotalTimeTaken() << " (ms)\n";
    os << "Average number of random solutions pulled: "
       << (((double)totalNumberOfRandomSolutionsPulled) / explorationPhases.size()) << "\n";
    os << "Total Number of random solutions pulled : " << totalNumberOfRandomSolutionsPulled
       << "\n";
    for(int i = 0; i < (int)nhc.neighbourhoods.size(); i++) {
      os << "Neighbourhood: " << nhc.neighbourhoods[i].name << "\n";
      os << indent << "Number activations: " << numberActivations[i] << "\n";
      u_int64_t averageTime = (numberActivations[i] > 0) ? totalTime[i] / numberActivations[i] : 0;
      os << indent << "Total time: " << totalTime[i] << "\n";
      os << indent << "Average time per activation: " << averageTime << "\n";
      os << indent << "Number positive solutions: " << numberPositiveSolutions[i] << "\n";
      os << indent << "Number negative solutions: " << numberNegativeSolutions[i] << "\n";
      os << indent << "Number no solutions: " << numberNoSolutions[i] << "\n";
      os << indent << "Number timeouts: " << numberTimeouts[i] << "\n";
    }
    os << "History of best solutions found "
       << "\n";
    for(auto& currentPair : bestSolutions) {
      os << "Value : " << currentPair.first << " Time : " << currentPair.second << " \n";
    }

    os << "Stats of Explorations:"
       << "\n";
    os << "---------------"
       << "\n";
    for(int i = 0; i < numberExplorationsByNHSize.size(); i++) {
      os << "NeighbourhoodSize " << (i + 1) << ":"
         << "\n";
      os << indent << "Activations: " << numberExplorationsByNHSize[i] << "\n";
      os << indent << "Number successful: " << numberSuccessfulExplorationsByNHSize[i] << "\n";
      os << indent << "Time Spent: " << neighbourhoodExplorationTimes[i] << "\n";
    }
    os << "---------------"
       << "\n";

    os << "Exploration Phases: "
       << "\n";
    for(int i = 0; i < explorationPhases.size(); i++) {
      os << "Phase " << (i + 1) << "\n";
      os << "------------"
         << "\n";
      os << "Start Time: " << explorationPhases[i].startExplorationTime << "\n";
      os << "End Time: " << explorationPhases[i].endExplorationTime << "\n";
      os << "Neighbourhood Size: " << explorationPhases[i].neighbourhoodSize << "\n";
      os << "Number of random solutions PUlled "
         << explorationPhases[i].numberOfRandomSolutionsPulled << "\n";
      os << "-----------------"
         << "\n";
    }
  }
};

#endif /* MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_ */
