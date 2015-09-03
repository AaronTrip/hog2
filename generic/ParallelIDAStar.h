//
//  ParallelIDA.h
//  hog2 glut
//
//  Created by Nathan Sturtevant on 9/2/15.
//  Copyright (c) 2015 University of Denver. All rights reserved.
//

#ifndef hog2_glut_ParallelIDA_h
#define hog2_glut_ParallelIDA_h

#include <iostream>
#include "SearchEnvironment.h"
#include <ext/hash_map>
#include "FPUtil.h"
#include "vectorCache.h"
#include "SharedQueue.h"
#include <thread>

const int workDepth = 2;

template <class action>
struct workUnit {
	action pre[workDepth];
	std::vector<action> solution;
	std::vector<int> gHistogram;
	double nextBound;
};

template <class environment, class state, class action>
class ParallelIDAStar {
public:
	ParallelIDAStar() { storedHeuristic = false;}
	virtual ~ParallelIDAStar() {}
	//	void GetPath(environment *env, state from, state to,
	//				 std::vector<state> &thePath);
	void GetPath(environment *env, state from, state to,
				 std::vector<action> &thePath);
	
	uint64_t GetNodesExpanded() { return nodesExpanded; }
	uint64_t GetNodesTouched() { return nodesTouched; }
	void ResetNodeCount() { nodesExpanded = nodesTouched = 0; }
	void SetHeuristic(Heuristic<state> *heur) { heuristic = heur; if (heur != 0) storedHeuristic = true;}
private:
	unsigned long long nodesExpanded, nodesTouched;
	
	void StartThreadedIteration(environment env, state startState, double bound);
	void DoIteration(environment *env,
					 action forbiddenAction, state &currState,
					 std::vector<action> &thePath, double bound, double g,
					 workUnit<action> &w);
	void GenerateWork(environment *env,
					  action forbiddenAction, state &currState,
					  std::vector<action> &thePath);
	
	void PrintGHistogram()
	{
		uint64_t early = 0, late = 0;
		for (int x = 0; x < gCostHistogram.size(); x++)
		{
			printf("%d\t%llu\n", x, gCostHistogram[x]);
			if (x*2 > gCostHistogram.size()-1)
				late += gCostHistogram[x];
			else
				early += gCostHistogram[x];
		}
		if (late < early)
			printf("Strong heuristic - Expect MM > A*\n");
		else
			printf("Weak heuristic - Expect MM >= MM0.\n");
	}
	void UpdateNextBound(double currBound, double fCost);
	state goal;
	double nextBound;
	vectorCache<action> actCache;
	bool storedHeuristic;
	Heuristic<state> *heuristic;
	std::vector<uint64_t> gCostHistogram;
	std::vector<workUnit<action>> work;
	std::vector<std::thread*> threads;
	SharedQueue<int> q;
	bool foundSolution;
};

//template <class state, class action>
//void ParallelIDAStar<environment, state, action>::GetPath(environment *env,
//											 state from, state to,
//											 std::vector<state> &thePath)
//{
//	assert(!"Not Implemented");
//}

template <class environment, class state, class action>
void ParallelIDAStar<environment, state, action>::GetPath(environment *env,
														  state from, state to,
														  std::vector<action> &thePath)
{
	int numThreads = std::thread::hardware_concurrency();
	if (!storedHeuristic)
		heuristic = env;
	nextBound = 0;
	nodesExpanded = nodesTouched = 0;
	foundSolution = false;
	thePath.resize(0);

	// Set class member
	goal = to;
	
	if (env->GoalTest(from, to))
		return;
	
	std::vector<action> act;
	env->GetActions(from, act);
	
	double rootH = heuristic->HCost(from, to);
	UpdateNextBound(0, rootH);
	
	// builds a list of all states at a fixed depth
	// we will then search them in parallel
	GenerateWork(env, act[0], from, thePath);
	printf("%lu pieces of work generated\n", work.size());
	
	while (!foundSolution)
	{
		gCostHistogram.clear();
		gCostHistogram.resize(nextBound+1);
		threads.resize(0);
		
		printf("Starting iteration with bound %f; %llu expanded, %llu generated\n", nextBound, nodesExpanded, nodesTouched);
		fflush(stdout);
		
		for (int x = 0; x < work.size(); x++)
		{
			q.Add(x);
		}
		for (int x = 0; x < numThreads; x++)
		{
			threads.push_back(new std::thread(&ParallelIDAStar<environment, state, action>::StartThreadedIteration, this,
												 *env, from, nextBound));
		}
		for (int x = 0; x < threads.size(); x++)
		{
			threads[x]->join();
			delete threads[x];
			threads[x] = 0;
		}
		double bestBound = nextBound*10; // FIXME: Better ways to do bounds
		for (int x = 0; x < work.size(); x++)
		{
			for (int y = 0; y < work[x].gHistogram.size(); y++)
			{
				gCostHistogram[y] += work[x].gHistogram[y];
			}
			if (work[x].solution.size() != 0)
			{
				thePath = work[x].solution;
			}
			if (work[x].nextBound > nextBound && work[x].nextBound < bestBound)
				bestBound = work[x].nextBound;
		}
		nextBound = bestBound;
		PrintGHistogram();
		if (thePath.size() != 0)
			return;
	}
}

template <class environment, class state, class action>
void ParallelIDAStar<environment, state, action>::GenerateWork(environment *env,
															   action forbiddenAction, state &currState,
															   std::vector<action> &thePath)
{
	if (thePath.size() >= workDepth)
	{
		workUnit<action> w;
		for (int x = 0; x < workDepth; x++)
		{
			w.pre[x] = thePath[x];
		}
		work.push_back(w);
		return;
	}
	
	std::vector<action> actions;
	env->GetActions(currState, actions);
	nodesTouched += actions.size();
	nodesExpanded++;
	int depth = (int)thePath.size();
	
	for (unsigned int x = 0; x < actions.size(); x++)
	{
		if ((depth != 0) && (actions[x] == forbiddenAction))
			continue;
		
		thePath.push_back(actions[x]);
		
		env->ApplyAction(currState, actions[x]);
		action a = actions[x];
		env->InvertAction(a);
		GenerateWork(env, a, currState, thePath);
		env->UndoAction(currState, actions[x]);
		thePath.pop_back();
	}
	
}

template <class environment, class state, class action>
void ParallelIDAStar<environment, state, action>::StartThreadedIteration(environment env, state startState, double bound)
{
	std::vector<action> thePath;
	while (true)
	{
		int nextValue;
		// All values put in before threads start. Once the queue is empty we're done
		if (q.Remove(nextValue) == false)
			break;
		
		thePath.resize(0);
		double g = 0;
		work[nextValue].solution.resize(0);
		work[nextValue].gHistogram.clear();
		work[nextValue].gHistogram.resize(bound+1);
		work[nextValue].nextBound = 10*bound;//FIXME: Better ways to do this
		for (int x = 0; x < workDepth; x++)
		{
			g += env.GCost(startState, work[nextValue].pre[x]);
			env.ApplyAction(startState, work[nextValue].pre[x]);
			thePath.push_back(work[nextValue].pre[x]);
		}
		action last = work[nextValue].pre[workDepth-1];
		env.InvertAction(last);
		
		if (fgreater(g+heuristic->HCost(startState, goal), bound))
		{
			work[nextValue].nextBound = g+heuristic->HCost(startState, goal);
		}
		else {
			DoIteration(&env, last, startState, thePath, bound, g, work[nextValue]);
		}
		
		for (int x = workDepth-1; x >= 0; x--)
		{
			env.UndoAction(startState, work[nextValue].pre[x]);
			g -= env.GCost(startState, work[nextValue].pre[x]);
		}
	}
}


template <class environment, class state, class action>
void ParallelIDAStar<environment, state, action>::DoIteration(environment *env,
															  action forbiddenAction, state &currState,
															  std::vector<action> &thePath, double bound, double g,
															  workUnit<action> &w)
{
	double h = heuristic->HCost(currState, goal);//, parentH); // TODO: restore code that uses parent h-cost
	
	if (fgreater(g+h, bound))
	{
		if (g+h < w.nextBound)
			w.nextBound = g+h;
		return;
	}

	// must do this after we check the f-cost bound
	if (env->GoalTest(currState, goal))
	{
		w.solution = thePath;
		foundSolution = true;
		return;
	}
	
	std::vector<action> actions;// = *actCache.getItem();
	env->GetActions(currState, actions);
	nodesTouched += actions.size();
	nodesExpanded++;
	w.gHistogram[g]++;

	for (unsigned int x = 0; x < actions.size(); x++)
	{
		if (actions[x] == forbiddenAction)
			continue;
		
		thePath.push_back(actions[x]);
		
		double edgeCost = env->GCost(currState, actions[x]);
		env->ApplyAction(currState, actions[x]);
		action a = actions[x];
		env->InvertAction(a);
		DoIteration(env, a, currState, thePath, bound, g+edgeCost, w);
		env->UndoAction(currState, actions[x]);
		thePath.pop_back();
		if (foundSolution)
			break;
	}
}


template <class environment, class state, class action>
void ParallelIDAStar<environment, state, action>::UpdateNextBound(double currBound, double fCost)
{
	if (!fgreater(nextBound, currBound))
	{
		nextBound = fCost;
		//printf("Updating next bound to %f\n", nextBound);
	}
	else if (fgreater(fCost, currBound) && fless(fCost, nextBound))
	{
		nextBound = fCost;
		//printf("Updating next bound to %f\n", nextBound);
	}
}

#endif
