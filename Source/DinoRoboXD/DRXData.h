// Static game data tables (single source of truth = Docs/*.md).
#pragma once

#include "DRXTypes.h"

namespace DRX
{
	const FItemDef& Item(EItem I);
	const FBuildingDef& Bld(EBuilding B);
	const FUnitDef& Unit(EUnit U);
	const FTechDef& Tech(ETech T);
	TArrayView<const FRecipe> Recipes();

	// Can a building of type B run recipe R? (furnaces run kiln recipes, etc.)
	bool BuildingRunsRecipe(EBuilding B, const FRecipe& R);

	// Which cyber forms a conversion rig / hatchery can queue, by family.
	EUnitFamily FamilyOf(EUnit U);

	inline bool IsDino(EUnit U) { return Unit(U).Family != EUnitFamily::Robot; }
}
