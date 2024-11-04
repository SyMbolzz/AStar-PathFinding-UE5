#include "PathFinding.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UPathFinding::UPathFinding()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UPathFinding::BeginPlay()
{
	Super::BeginPlay();
	
	InitCells();
}

// Called every frame
void UPathFinding::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DrawCells();
}

//////////////////////

void UPathFinding::ToggleWall(FVector WorldCoord)
{
	if (!bPathStarted) // Only toggle walls if pathfinding hasn't started
	{
		ToggleWall(GetCellCoord({ WorldCoord.X, WorldCoord.Y }));
	}
}

void UPathFinding::ToggleBeginEnd(FVector WorldCoord)
{
	if (!bPathStarted) // Only toggle start/end if pathfinding hasn't started
	{
		ToggleBeginEnd(GetCellCoord({ WorldCoord.X, WorldCoord.Y }));
	}
}

void UPathFinding::NextIteration()
{
	if (bPathEnded) // Exit if pathfinding is already completed
	{
		return;
	}

	if (!bPathStarted)
	{
		if (StartCell && EndCell) // Begin pathfinding if both start and end cells are set
		{
			WeightSurroundingCells(StartCoord);
			bPathStarted = true;
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Please place start and end positions (Right click)")));
		}
		return;
	}

	switch (SelectLightestCell())
	{
	case 1: // Path found
		SelectFinalPath();
		bPathEnded = true;
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Path found")));
		break;
	case 2: // No path found
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("No path was found")));
		break;
	}
}

//////////////////////

void UPathFinding::InitCells()
{
	int CellNumber = HorizontalCells * VerticalCells;
	Cells.Reserve(CellNumber); // Reserve memory for all cells to improve efficiency

	for (int i = 0; i < CellNumber; i++)
	{
		Cells.Add(new S_Cell); // Add a new cell to the grid
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("There are %i cells"), CellNumber));
}

void UPathFinding::DrawCells()
{
	for (int j = 0; j < VerticalCells; j++)
	{
		for (int i = 0; i < HorizontalCells; i++)
		{
			S_Cell* Cell = GetCell({ i, j });
			if (!Cell)
			{
				return;
			}

			float Height = 0.f;
			FVector Center = { i * CellSize, -j * CellSize, Height };
			// Draw plane to represent cell with color based on cell state
			UKismetSystemLibrary::DrawDebugPlane(GetWorld(), FPlane(0.f, 0.f, -1.f, 0.f), Center, (CellSize / 2) * 0.9f, GetColor({ i, j }));

			// Display cell weights or start/end labels
			if (Cell == StartCell)
			{
				UKismetSystemLibrary::DrawDebugString(GetWorld(), Center, FString("A"));
			}
			else if (Cell == EndCell)
			{
				UKismetSystemLibrary::DrawDebugString(GetWorld(), Center, FString("B"));
			}
			else if (Cell->ColorNum != 0 && Cell->ColorNum != 1)
			{
				// Draw weight
				UKismetSystemLibrary::DrawDebugString(GetWorld(), Center, FString::FromInt(GetCell({ i, j })->Weight));
				
				// Draw arrow to parent cell
				if (Cell->Parent)
				{
					FVector ParentCenter = { GetCellCoord(Cell->Parent).X * CellSize, -GetCellCoord(Cell->Parent).Y * CellSize, Height };
					FVector MiddlePoint = (Center + ParentCenter) / 2;
					UKismetSystemLibrary::DrawDebugArrow(GetWorld(), Center, MiddlePoint, CellSize, FColor::White);
				}
			}
		}
	}
}

//////////////////////

UPathFinding::S_Cell* UPathFinding::GetCell(S_Coord Coord)
{
	if (Coord.X >= HorizontalCells || Coord.X < 0 || Coord.Y >= VerticalCells || Coord.Y < 0)
	{
		return nullptr; // Return null if coordinates are out of bounds
	}

	int Index = Coord.Y * HorizontalCells + Coord.X;

	if (Index < 0 || Index >= Cells.Num())
	{
		return nullptr;
	}

	return Cells[Index];
}

FColor UPathFinding::GetColor(S_Coord Coord)
{
	FColor color = FColor::White; // Default unvisited cell color
	S_Cell* Cell = GetCell(Coord);

	if (!Cell)
	{
		return color;
	}

	switch (GetCell(Coord)->ColorNum)
	{
	case 1: // Wall color
		color = FColor::Black;
		break;
	case 2: // Weighted cell color
		color = FColor::Green;
			break;
	case 3: // Selected cell color
		color = FColor::Red;
			break;
	case 4: // Final path cell color
		color = FColor::Blue;
			break;
	}

	return color;
}

UPathFinding::S_Coord UPathFinding::GetCellCoord(FVector2D WorldCoord)
{
	S_Coord Coord = { FMath::Floor((WorldCoord.X + CellSize / 2.f) / CellSize), 
					  FMath::Floor((-WorldCoord.Y + CellSize / 2.f) / CellSize) };
	return Coord;
}

UPathFinding::S_Coord UPathFinding::GetCellCoord(S_Cell* Cell)
{
	int Index = Cells.Find(Cell);
	int X = Index % HorizontalCells;
	int Y = (Index - X) / HorizontalCells;
	return { X, Y };
}

void UPathFinding::ToggleWall(S_Coord Coord)
{
	S_Cell* Cell = GetCell(Coord);

	if (!Cell || StartCell == Cell || EndCell == Cell)
	{
		return;
	}

	if (Cell->ColorNum == 1)
	{
		Cell->ColorNum = 0;
		return;
	}

	if (Cell->ColorNum == 0)
	{
		Cell->ColorNum = 1;
		return;
	}
}

void UPathFinding::ToggleBeginEnd(S_Coord Coord)
{
	S_Cell* Cell = GetCell(Coord);

	if (!Cell)
	{
		return;
	}

	if (Cell->ColorNum == 1) // Ignore if cell is a wall
	{
		return;
	}

	// Set as start if start isn't set
	if (!StartCell && Cell != EndCell)
	{
		StartCell = Cell;
		Cell->ColorNum = 4;
		Cell->StartDist = 0;
		StartCoord = Coord;
		return;
	}

	// Set as end if end isn't set
	if (!EndCell && Cell != StartCell)
	{
		EndCell = Cell;
		Cell->ColorNum = 4;
		EndCoord = Coord;
		return;
	}

	// Remove start cell
	if (Cell == StartCell)
	{
		StartCell = nullptr;
		Cell->ColorNum = 0;
		Cell->StartDist = -1;
		return;
	}

	// Remove end cell
	if (Cell == EndCell)
	{
		EndCell = nullptr;
		Cell->ColorNum = 0;
		return;
	}
}

void UPathFinding::SelectFinalPath()
{
	S_Cell* Cell = SelectedCells.Last();
	while (Cell)
	{
		Cell->ColorNum = 4;
		Cell = Cell->Parent; // Move to the parent cell along the path
	}
}

// Might want to use std::priority_queue
int UPathFinding::SelectLightestCell()
{
	if (WeightedCells.Num() == 0)
	{
		return 2; // Return 2 if no cells to select
	}

	S_Cell* LightestCell = WeightedCells[0];

	// Find cell with the minimum weight
	for (int i = 0; i < WeightedCells.Num(); i++)
	{
		if (LightestCell->Weight > WeightedCells[i]->Weight)
		{
			LightestCell = WeightedCells[i];
		}
		// Optional
		else if (LightestCell->Weight == WeightedCells[i]->Weight)
		{
			int LightDistanceToEnd = CalculateDistance(GetCellCoord(LightestCell), EndCoord);
			int OtherDistanceToEnd = CalculateDistance(GetCellCoord(WeightedCells[i]), EndCoord);
			if (LightDistanceToEnd > OtherDistanceToEnd)
			{
				LightestCell = WeightedCells[i];
			}
		}
	}

	// Move the lightest cell to SelectedCells
	SelectedCells.Add(LightestCell);
	LightestCell->ColorNum = 3;
	WeightedCells.Remove(LightestCell);

	if (WeightSurroundingCells(GetCellCoord(LightestCell)))
	{
		return 1; // Return 1 if the end cell is reached
	}
	
	return 0;
}

bool UPathFinding::WeightSurroundingCells(S_Coord CenterCoord)
{
	// Iterate over neighboring directions and evaluate each surrounding cell
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			if (i == 0 && j == 0)
			{
				continue; // Skip the center cell
			}

			S_Cell* Cell = GetCell({ CenterCoord.X + i, CenterCoord.Y + j });

			if (!Cell)
			{
				continue;
			}

			if (Cell == EndCell)
			{
				return true; // End found
			}

			if (Cell->ColorNum != 0 && Cell->ColorNum != 2)
			{
				continue; // Skip walls and already selected cells
			}
			
			EvaluateWeight(CenterCoord, { CenterCoord.X + i, CenterCoord.Y + j });

			// Add to weighted cells if it wasn't already in
			if (!WeightedCells.Contains(Cell))
			{
				WeightedCells.Add(Cell);
			}
		}
	}

	return false;
}

void UPathFinding::EvaluateWeight(S_Coord EvaluaterCoord, S_Coord EvaluatedCoord)
{
	S_Cell* EvaluaterCell = GetCell(EvaluaterCoord);
	S_Cell* EvaluatedCell = GetCell(EvaluatedCoord);

	if (!EvaluaterCell || !EvaluatedCell)
	{
		return;
	}

	// Calculate start distance then update cell weight
	int NewStartDist = EvaluaterCell->StartDist + CalculateDistance(EvaluaterCoord, EvaluatedCoord);

	if (EvaluatedCell->StartDist < 0 || NewStartDist < EvaluatedCell->StartDist)
	{
		EvaluatedCell->StartDist = NewStartDist;
		EvaluatedCell->Weight = EvaluatedCell->StartDist + CalculateDistance(EvaluatedCoord, EndCoord);
		EvaluatedCell->Parent = EvaluaterCell;
		EvaluatedCell->ColorNum = 2;
	}
}

int UPathFinding::CalculateDistance(S_Coord Coord1, S_Coord Coord2)
{
	// Distance of horizontal/vertical move
	int DistanceHorVer = 10;
	// Distance of diagonal move, set slightly higher than DistanceHorVer
	int DistanceDiag = 14;

	int DifX = FMath::Abs(Coord1.X - Coord2.X);
	int DifY = FMath::Abs(Coord1.Y - Coord2.Y);

	// Determine how many diagonal moves are possible
	int MovDiag = FMath::Min(DifX, DifY);
	// Determine remaining moves that can only be horizontal or vertical
	int MovHorVer = FMath::Abs(DifX - DifY);

	// Calculate total distance as the sum of diagonal moves and remaining horizontal/vertical moves
	int Distance = MovDiag * DistanceDiag + MovHorVer * DistanceHorVer;

	return Distance;
}

void UPathFinding::ResetCells()
{
	// Loop through each cell in the grid and reset its properties
	for (int i = 0; i < Cells.Num(); i++)
	{
		S_Cell* Cell = Cells[i];
		Cell->Weight = -1;
		Cell->StartDist = -1;
		Cell->ColorNum = 0;
		Cell->Parent = nullptr;
	}

	StartCell = nullptr;
	EndCell = nullptr;
	
	WeightedCells.Empty();
	SelectedCells.Empty();

	bPathStarted = false;
	bPathEnded = false;
}
