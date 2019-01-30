#include "vcScene.h"

#include "vcState.h"
#include "vcRender.h"

void vcSceneItem::AddItem(vcState *pProgramState)
{
  vcFolder *pParent = pProgramState->sceneExplorer.clickedItem.pParent;
  vcSceneItem *pChild = nullptr;

  if (pParent)
    pChild = pParent->children[pProgramState->sceneExplorer.clickedItem.index];

  // TODO: Proper Exception Handling
  if (pChild != nullptr && pChild->type == vcSOT_Folder)
    ((vcFolder*)pChild)->children.push_back(this);
  else if (pParent != nullptr)
    pParent->children.push_back(this);
  else
    pProgramState->sceneExplorer.pItems->children.push_back(this);
}

void vcScene_RemoveReference(vcSceneItemRef *pItemRef, vcFolder *pParent, size_t index)
{
  if (pItemRef->pParent == pParent && pItemRef->index == index)
  {
    pItemRef->pParent = nullptr;
    pItemRef->index = SIZE_MAX;
  }
  else if (pItemRef->pParent == pParent && pItemRef->index > index)
  {
    --pItemRef->index;
  }
}

void vcScene_RemoveItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  // Remove references
  vcScene_RemoveReference(&pProgramState->sceneExplorer.insertItem, pParent, index);
  vcScene_RemoveReference(&pProgramState->sceneExplorer.clickedItem, pParent, index);
  for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
  {
    if (pProgramState->sceneExplorer.selectedItems[i].pParent == pParent && pProgramState->sceneExplorer.selectedItems[i].index == index)
    {
      pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
      --i;
    }
    else if (pProgramState->sceneExplorer.selectedItems[i].pParent == pParent && pProgramState->sceneExplorer.selectedItems[i].index > index)
    {
      --pProgramState->sceneExplorer.selectedItems[i].index;
    }
  }

  if (pParent->children[index]->loadStatus == vcSLS_Pending)
    udInterlockedCompareExchange(&pParent->children[index]->loadStatus, vcSLS_Unloaded, vcSLS_Pending);

  while (pParent->children[index]->loadStatus == vcSLS_Loading)
    udYield(); // Spin until other thread stops processing

  if (pParent->children[index]->loadStatus == vcSLS_Loaded || pParent->children[index]->loadStatus == vcSLS_OpenFailure || pParent->children[index]->loadStatus == vcSLS_Failed)
  {
    pParent->children[index]->Cleanup(pProgramState);

    if (pParent->children[index]->pMetadata)
      pParent->children[index]->pMetadata->Destroy();

    udFree(pParent->children[index]->pMetadata);
    udFree(pParent->children[index]->pOriginalZone);
    udFree(pParent->children[index]->pZone);
  }

  pParent->children[index]->loadStatus = vcSLS_Unloaded;

  udFree(pParent->children.at(index));
  pParent->children.erase(pParent->children.begin() + index);
}

void vcScene_RemoveAll(vcState *pProgramState)
{
  if (pProgramState->sceneExplorer.pItems == nullptr)
    return;

  while (pProgramState->sceneExplorer.pItems->children.size() > 0)
    vcScene_RemoveItem(pProgramState, pProgramState->sceneExplorer.pItems, 0);
  pProgramState->sceneExplorer.selectedItems.clear();

  vcRender_ClearTiles(pProgramState->pRenderContext);
}

void vcScene_RemoveSelected(vcState *pProgramState, vcFolder *pFolder)
{
  for (size_t i = 0; i < pFolder->children.size(); ++i)
  {
    if (pFolder->children[i]->selected)
    {
      vcScene_RemoveItem(pProgramState, pFolder, i);
      i--;
      continue;
    }

    if (pFolder->children[i]->type == vcSOT_Folder)
      vcScene_RemoveSelected(pProgramState, (vcFolder*)pFolder->children[i]);
  }
}

void vcScene_RemoveSelected(vcState *pProgramState)
{
  vcScene_RemoveSelected(pProgramState, pProgramState->sceneExplorer.pItems);
  pProgramState->sceneExplorer.selectedItems.clear();
}

bool vcScene_ContainsItem(vcFolder *pParent, vcSceneItem *pItem)
{
  for (size_t i = 0; i < pParent->children.size(); ++i)
  {
    if (pParent->children[i] == pItem)
      return true;

    if (pParent->children[i]->type == vcSOT_Folder)
      if (vcScene_ContainsItem((vcFolder*)pParent->children[i], pItem))
        return true;
  }

  return pParent == pItem;
}

void vcScene_SelectItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  if (!pParent->children[index]->selected)
  {
    pParent->children[index]->selected = true;
    pProgramState->sceneExplorer.selectedItems.push_back({ pParent, index });
  }
}

void vcScene_UnselectItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  if (pParent->children[index]->selected)
  {
    pParent->children[index]->selected = false;
    for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
    {
      const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
      if (item.pParent == pParent && item.index == index)
        pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
    }
  }
}

void vcScene_ClearSelection(vcFolder *pParent)
{
  for (size_t i = 0; i < pParent->children.size(); i++)
  {
    if (pParent->children[i]->type == vcSOT_Folder)
      vcScene_ClearSelection((vcFolder*)pParent->children[i]);
    else
      pParent->children[i]->selected = false;
  }
  pParent->selected = false;
}

void vcScene_ClearSelection(vcState *pProgramState)
{
  vcScene_ClearSelection(pProgramState->sceneExplorer.pItems);
  pProgramState->sceneExplorer.selectedItems.clear();
}

void vcScene_UpdateItemToCurrentProjection(vcState *pProgramState, vcSceneItem *pModel)
{
  if (!pModel)
  {
    for (size_t i = 0; i < pProgramState->sceneExplorer.pItems->children.size(); ++i)
      vcScene_UpdateItemToCurrentProjection(pProgramState, pProgramState->sceneExplorer.pItems->children[i]);
  }
  else
  {
    // Handle transforming into the camera's GeoZone
    if (pProgramState->gis.isProjected && pModel->pZone != nullptr && pProgramState->gis.SRID != pModel->pZone->srid)
    {
      pModel->sceneMatrix = udGeoZone_TransformMatrix(pModel->sceneMatrix, *pModel->pZone, pProgramState->gis.zone);

      // This is not ideal as it will gather drift
      memcpy(pModel->pZone, &pProgramState->gis.zone, sizeof(pProgramState->gis.zone));
    }
  }
}

bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  if ((pModel->pZone != nullptr && vcGIS_ChangeSpace(&pProgramState->gis, pModel->pOriginalZone->srid)) || (pModel->pZone == nullptr && vcGIS_ChangeSpace(&pProgramState->gis, 0)))
    vcScene_UpdateItemToCurrentProjection(pProgramState, nullptr); // Update all models to new zone

  pProgramState->pCamera->position = vcScene_GetItemWorldSpacePivotPoint(pModel);

  return true;
}

udDouble3 vcScene_GetItemWorldSpacePivotPoint(vcSceneItem *pModel)
{
  udDouble3 midPoint = udDouble3::zero();

  if (pModel != nullptr)
    midPoint = (pModel->sceneMatrix * udDouble4::create(pModel->pivot, 1.0)).toVector3();

  return midPoint;
}
