#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PathFinding.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TP1_API UPathFinding : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPathFinding();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Number of horizontal cells in the grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (UIMin = 1))
	int HorizontalCells = 10;

	// Number of vertical cells in the grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (UIMin = 1))
	int VerticalCells = 10;

	// Size of each square cell in the grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (UIMin = 0))
	float CellSize = 100.f;


	UFUNCTION(BlueprintCallable)
	void ToggleWall(FVector WorldCoord);

	UFUNCTION(BlueprintCallable)
	void ToggleBeginEnd(FVector WorldCoord);

	UFUNCTION(BlueprintCallable)
	void NextIteration();

	UFUNCTION(BlueprintCallable)
	void ResetCells();

private:

	// Coordinates struct to hold grid position (X, Y)
	struct S_Coord
	{
		int X = 0;
		int Y = 0;
	};

	// Cell struct to hold pathfinding data
	struct S_Cell
	{
		int Weight = -1;
		int StartDist = -1;
		int ColorNum = 0;
		S_Cell* Parent = nullptr;
	};

	// Array holding pointers to all cells in the grid
	TArray<S_Cell*> Cells = {};

	S_Cell* StartCell = nullptr;
	S_Coord StartCoord;

	S_Cell* EndCell = nullptr;
	S_Coord EndCoord;

	// Weighted and selected cells for pathfinding
	TArray<S_Cell*> WeightedCells = {};
	TArray<S_Cell*> SelectedCells = {};

	// Flags indicating if pathfinding has started and ended
	bool bPathStarted = false;
	bool bPathEnded = false;

	// Initializes all cells in the grid
	void InitCells();
	// Draws the cells in the world
	void DrawCells();
	// Retrieves a cell based on its coordinates
	S_Cell* GetCell(S_Coord Coord);
	// Gets the color of a cell for visualization purposes
	FColor GetColor(S_Coord Coord);

	// Converts world coordinates to grid coordinates
	S_Coord GetCellCoord(FVector2D WorldCoord);
	// Retrieves the coordinates of a specific cell
	S_Coord GetCellCoord(S_Cell* Cell);

	// Toggles the wall state of a cell
	void ToggleWall(S_Coord Coord);
	// Toggles start or end state of a cell
	void ToggleBeginEnd(S_Coord Coord);

	// Finalizes the path by marking the cells from end to start
	void SelectFinalPath();
	// Selects the cell with the lightest weight from WeightedCells
	int SelectLightestCell();

	// Weights surrounding cells
	bool WeightSurroundingCells(S_Coord CenterCoord);
	// Evaluates and updates weight of a specific cell based on its neighbor
	void EvaluateWeight(S_Coord EvaluaterCoord, S_Coord EvaluatedCoord);

	//Calculate distance only going horizontal, vertical and diagonal in a grid
	int CalculateDistance(S_Coord Coord1, S_Coord Coord2);
};
