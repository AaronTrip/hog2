/*
 *  Driver.cpp
 *  hog
 *
 *  Created by Nathan Sturtevant on 10/1/19.
 *
 *  This file is part of HOG.
 */

#include <cstring>
#include "Common.h"
#include "PermutationPDB.h"
#include "LexPermutationPDB.h"
#include "MR1PermutationPDB.h"
#include "Driver.h"
#include "MNPuzzle.h"
#include "RubiksCube.h"
#include "Timer.h"
#include "STPInstances.h"

template <int width, int height>
void BuildSTPPDB();
void BuildRC();

int main(int argc, char* argv[])
{
	InstallHandlers();
	RunHOGGUI(argc, argv, 640, 640);
	return 0;
}

enum Domain {
	kRC,
	kSTP44,
	kSTP55
};
Domain domain;
bool additive = false;
std::vector<int> pattern;
std::string path;

/**
 * Allows you to install any keyboard handlers needed for program interaction.
 */
void InstallHandlers()
{
	InstallWindowHandler(MyWindowHandler);
	InstallCommandLineHandler(MyCLHandler, "-domain", "-domain", "Select domain to build for. {STP44 STP55 RC}");
	InstallCommandLineHandler(MyCLHandler, "-pattern", "-pattern", "Choose tiles in PDB");
	InstallCommandLineHandler(MyCLHandler, "-additive", "-additive", "Build additive PDB");
	InstallCommandLineHandler(MyCLHandler, "-path", "-path", "Set path for output PDB");
}

void MyWindowHandler(unsigned long windowID, tWindowEventType eType)
{
	if (eType == kWindowDestroyed)
	{
		printf("Window %ld destroyed\n", windowID);
	}
	else if (eType == kWindowCreated)
	{
		printf("Building PDB\n");
		switch (domain)
		{
			case kRC:
				BuildRC();
				break;
			case kSTP44:
				BuildSTPPDB<4, 4>();
				break;
			case kSTP55:
				BuildSTPPDB<5, 5>();
				break;
		}
		printf("Done.\n");
		exit(0);
	}
}

int MyCLHandler(char *argument[], int maxNumArgs)
{
	if (strcmp(argument[0], "-domain") == 0)
	{
		if (maxNumArgs <= 1)
		{
			printf("Need domain: STP44 STP55 RC\n");
			exit(0);
		}
		if (strcmp(argument[1], "RC") == 0)
			domain = kRC;
		else if (strcmp(argument[1], "STP44") == 0)
			domain = kSTP44;
		if (strcmp(argument[1], "STP55") == 0)
			domain = kSTP55;
		return 2;
	}
	if (strcmp(argument[0], "-additive") == 0)
	{
		additive = true;
		return 1;
	}

	if (strcmp(argument[0], "-pattern") == 0)
	{
		int cnt = 1;
		maxNumArgs--;
		while (maxNumArgs > 0 && argument[cnt][0] != '-')
		{
			maxNumArgs--;
			pattern.push_back(atoi(argument[cnt]));
			cnt++;
		}
		return cnt;
	}

	return 1;
}

template <int width, int height>
void BuildSTPPDB()
{
	MNPuzzle<width, height> mnp;
	MNPuzzleState<width, height> goal;
	std::vector<slideDir> moves;
	goal.Reset();
	mnp.StoreGoal(goal);

	LexPermutationPDB<MNPuzzleState<width, height>, slideDir, MNPuzzle<width, height>> pdb(&mnp, goal, pattern);
	if (additive)
	{
		mnp.SetPattern(pattern);
		pdb.BuildAdditivePDB(goal, 1); // parallelism not fixed yet
		pdb.Save(path.c_str());
	}
	else {
		pdb.BuildPDB(goal, std::thread::hardware_concurrency());
		pdb.Save(path.c_str());
	}
}

void BuildRC()
{
	RubiksCube cube;
	RubiksState goal;
	goal.Reset();
	
	std::vector<int> edges;
	std::vector<int> corners;
	for (auto i : pattern)
	{
		if (i < 12)
		{
			edges.push_back(i);
		}
		else {
			corners.push_back(i-12);
		}
	}
	printf("\nEdges: ");
	for (auto i : edges) printf("%d ", i);
	printf("\nCorners: ");
	for (auto i : corners) printf("%d ", i);
	printf("\n");
	
	RubikPDB pdb(&cube, goal, edges, corners);
	pdb.BuildPDB(goal, std::thread::hardware_concurrency());
	pdb.Save(path.c_str());
}
