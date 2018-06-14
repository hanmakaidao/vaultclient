#include "vcSettings.h"

#include "udPlatform/udValue.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"
#include "imgui_ex/imgui_dock_internal.h"

extern ImGui::DockContext g_dock;
const char *pDefaultSettings = R"config({"window":{"width":1280,"height":720,"maximized":false,"fullscreen":false},"frames":{"scene":true,"settings":true,"explorer":true},"camera":{"moveSpeed":10,"nearPlane":0.5,"farPlane":10000,"fieldOfView":0.872665,"moveMode":0},"maptiles":{"enabled":true,"blendMode":0,"transparency":1.0,"mapHeight":0.0},"docks":[{"label":"ROOT","child":[1,2],"prev":-1,"next":-1,"parent":-1,"status":0,"active":true,"open":false,"position":{"x":0,"y":22},"size":{"x":1280,"y":673},"location":"-1"},{"label":"Scene","child":[-1,-1],"prev":-1,"next":-1,"parent":0,"status":0,"active":true,"open":true,"position":{"x":0,"y":22},"size":{"x":976,"y":673},"location":"1"},{"label":"DOCK","child":[3,4],"prev":-1,"next":-1,"parent":0,"status":0,"active":true,"open":false,"position":{"x":976,"y":22},"size":{"x":304,"y":673},"location":"0"},{"label":"Scene Explorer","child":[-1,-1],"prev":-1,"next":-1,"parent":2,"status":0,"active":true,"open":true,"position":{"x":976,"y":22},"size":{"x":304,"y":442},"location":"20"},{"label":"Settings","child":[-1,-1],"prev":-1,"next":-1,"parent":2,"status":0,"active":true,"open":true,"position":{"x":976,"y":464},"size":{"x":304,"y":231},"location":"30"}]})config";

int vcSettings_RecursiveSaveDock(udValue &settings, ImGui::DockContext::Dock *pParentDock, int dockIndex, int parentIndex, int prevIndexIn = -1);

void vcSettings_LoadDocks(udValue &settings)
{
  for (int i = 0; i < g_dock.m_docks.size(); ++i)
  {
    g_dock.m_docks[i]->~Dock();
    MemFree(g_dock.m_docks[i]);
  }
  g_dock.m_docks.clear();

  int totalDocks = (int)settings.Get("docks").ArrayLength();

  for (int i = 0; i < totalDocks; i++)
  {
    ImGui::DockContext::Dock *new_dock = (ImGui::DockContext::Dock*)ImGui::MemAlloc(sizeof(ImGui::DockContext::Dock));
    IM_PLACEMENT_NEW(new_dock) ImGui::DockContext::Dock();
    g_dock.m_docks.push_back(new_dock);
  }

  for (int i = 0; i < totalDocks; i++)
  {
    const udValue &dock = settings.Get("docks[%d]", i);

    g_dock.m_docks[i]->label = ImStrdup(dock.Get("label").AsString());
    g_dock.m_docks[i]->id = ImHash(g_dock.m_docks[i]->label, 0);

    g_dock.m_docks[i]->children[0] = g_dock.getDockByIndex(dock.Get("child[0]").AsInt());
    g_dock.m_docks[i]->children[1] = g_dock.getDockByIndex(dock.Get("child[1]").AsInt());
    g_dock.m_docks[i]->prev_tab = g_dock.getDockByIndex(dock.Get("prev").AsInt());
    g_dock.m_docks[i]->next_tab = g_dock.getDockByIndex(dock.Get("next").AsInt());
    g_dock.m_docks[i]->parent = g_dock.getDockByIndex(dock.Get("parent").AsInt());
    g_dock.m_docks[i]->status = (ImGui::DockContext::Status_)dock.Get("status").AsInt();
    g_dock.m_docks[i]->active = dock.Get("active").AsBool();
    g_dock.m_docks[i]->opened = dock.Get("open").AsBool();

    g_dock.m_docks[i]->pos.x = dock.Get("position.x").AsFloat();
    g_dock.m_docks[i]->pos.y = dock.Get("position.y").AsFloat();
    g_dock.m_docks[i]->size.x = dock.Get("size.x").AsFloat();
    g_dock.m_docks[i]->size.y = dock.Get("size.y").AsFloat();

    udStrcpy(g_dock.m_docks[i]->location, sizeof(g_dock.m_docks[i]->location), dock.Get("location").AsString(""));

    g_dock.tryDockToStoredLocation(*g_dock.m_docks[i]);
  }
}

void vcSettings_SaveDocks(udValue &settings)
{
  int dockIndex = 0;
  for (int i = 0; i < g_dock.m_docks.size(); ++i)
  {
    ImGui::DockContext::Dock& dock = *g_dock.m_docks[i];
    bool dockIsValidRoot = (g_dock.getDockIndex(dock.parent) == -1);

    if ((!dock.children[0] || !dock.children[1]) && udStrlen(dock.label) == 0) // if either of the children are nullptr and the label has a length of 0
      dockIsValidRoot = false; // is a root dock with no children, do not save

    if (dockIsValidRoot)
      dockIndex += vcSettings_RecursiveSaveDock(settings, &dock, dockIndex, -1);
  }
}

int vcSettings_GetChildDockCount(ImGui::DockContext::Dock *pDock) // includes next_tab's as children
{
  if (pDock)
    return (pDock->children[0] ? 1 + vcSettings_GetChildDockCount(pDock->children[0]) : 0)
    + (pDock->children[1] ? 1 + vcSettings_GetChildDockCount(pDock->children[1]) : 0)
    + (pDock->next_tab ? 1 + vcSettings_GetChildDockCount(pDock->next_tab) : 0);
  else
    return 0;
}

int vcSettings_RecursiveSaveDock(udValue &settings, ImGui::DockContext::Dock *pParentDock, int dockIndex, int parentIndex, int prevIndexIn /* = -1*/)
{
  int numChildren = 0;
  udValue dockJ;
  dockJ.SetObject();

  ImGui::DockContext::Dock &dock = *pParentDock;

  int i = g_dock.getDockIndex(pParentDock);


  int child1NewIndex = dock.children[0] ? dockIndex + 1 : -1;
  int child2NewIndex = dock.children[1] ? dockIndex + 2 + vcSettings_GetChildDockCount(dock.children[0]) : -1;

  int prevNewIndex = dock.prev_tab ? prevIndexIn : -1;
  int nextNewIndex = -1;
  if (child1NewIndex == -1 && child2NewIndex == -1) // cant have next tab and children, so else is unnecessary
  {
    // has no children
    nextNewIndex = dock.next_tab ? dockIndex + 1 : -1;
  }

  int child1Index = g_dock.getDockIndex(g_dock.m_docks[i]->children[0]);
  int child2Index = g_dock.getDockIndex(g_dock.m_docks[i]->children[1]);

  int nextIndex = g_dock.getDockIndex(g_dock.m_docks[i]->next_tab);

  dockJ.Set("label = '%s'", dock.parent ? (dock.label[0] == '\0' ? "DOCK" : dock.label) : (dock.label[0] == '\0' ? "ROOT" : dock.label));

  dockJ.Set("child[0] = %d", child1NewIndex);
  dockJ.Set("child[1] = %d", child2NewIndex);
  dockJ.Set("prev = %d", prevNewIndex);
  dockJ.Set("next = %d", nextNewIndex);
  dockJ.Set("parent = %d", parentIndex);

  dockJ.Set("status = %d", g_dock.m_docks[i]->status);
  dockJ.Set("active = %s", g_dock.m_docks[i]->active ? "true" : "false");
  dockJ.Set("open = %s", g_dock.m_docks[i]->opened ? "true" : "false");

  dockJ.Set("position.x = %f", g_dock.m_docks[i]->pos.x);
  dockJ.Set("position.y = %f", g_dock.m_docks[i]->pos.y);
  dockJ.Set("size.x = %f", g_dock.m_docks[i]->size.x);
  dockJ.Set("size.y = %f", g_dock.m_docks[i]->size.y);

  g_dock.fillLocation(dock);
  if(udStrlen(dock.location))
    dockJ.Set("location = '%s'", g_dock.m_docks[i]->location);

  settings.Set(&dockJ, "docks[]");

  if (child1Index >= 0 && child2Index >= 0)
  {
    numChildren += vcSettings_RecursiveSaveDock(settings, g_dock.m_docks[child1Index], dockIndex + 1, dockIndex);
    numChildren += vcSettings_RecursiveSaveDock(settings, g_dock.m_docks[child2Index], dockIndex + numChildren + 1, dockIndex);
  }

  if (nextIndex >= 0)
    numChildren += vcSettings_RecursiveSaveDock(settings, g_dock.m_docks[nextIndex], dockIndex + 1, parentIndex, dockIndex);

  return numChildren+1;
}

void vcSettings_InitializePrefPath(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage)
    return;

  char *pPath = SDL_GetPrefPath("euclideon", "client");

  if (pPath != nullptr)
    pSettings->pSaveFilePath = pPath;
  else
    pSettings->noLocalStorage = true;
}

const char* SDL_fileread(const char* filename)
{
  SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
  if (rw == NULL)
    return NULL;

  Sint64 res_size = SDL_RWsize(rw);
  char *res = udAllocType(char, res_size + 1, udAF_None);

  Sint64 nb_read_total = 0, nb_read = 1;
  char* buf = res;

  while (nb_read_total < res_size && nb_read != 0)
  {
    nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
    nb_read_total += nb_read;
    buf += nb_read;
  }

  SDL_RWclose(rw);

  if (nb_read_total != res_size)
  {
    udFree(res);
    return nullptr;
  }

  res[nb_read_total] = '\0';
  return res;
}

bool SDL_filewrite(const char* filename, const char *pStr, size_t bytes)
{
  bool success = false;

  SDL_RWops *rw = SDL_RWFromFile(filename, "wb");
  if (rw != NULL)
  {
    if (SDL_RWwrite(rw, pStr, 1, bytes) != bytes)
      success = false;
    else
      success = true;

    SDL_RWclose(rw);
  }

  return success;
}

bool vcSettings_Load(vcSettings *pSettings, bool forceReset /*= false*/)
{
  ImGui::GetIO().IniFilename = NULL; // Disables auto save and load

  const char *pSavedData = nullptr;

  if (!pSettings->noLocalStorage && pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  if (pSettings->noLocalStorage || pSettings->pSaveFilePath == nullptr || forceReset)
  {
    pSavedData = pDefaultSettings;
  }
  else
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

    const char *pSettingFileData = SDL_fileread(buffer);

    if (pSettingFileData != nullptr)
      pSavedData = pSettingFileData;
    else
      pSavedData = pDefaultSettings;
  }

  if (pSavedData != nullptr)
  {
    udValue data;
    data.Parse(pSavedData);

    // Misc Settings
    pSettings->showFPS = data.Get("showFPS").AsBool(false);

    // Windows
    pSettings->window.xpos = data.Get("window.position.x").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.ypos = data.Get("window.position.y").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.width = data.Get("window.width").AsInt(1280);
    pSettings->window.height = data.Get("window.height").AsInt(720);
    pSettings->window.maximized = data.Get("window.maximized").AsBool(false);
    pSettings->window.fullscreen = data.Get("window.fullscreen").AsBool(false);

    pSettings->window.windowsOpen[vcdScene] = data.Get("frames.scene").AsBool(true);
    pSettings->window.windowsOpen[vcdSettings] = data.Get("frames.settings").AsBool(true);
    pSettings->window.windowsOpen[vcdSceneExplorer] = data.Get("frames.explorer").AsBool(true);

    // Camera
    pSettings->camera.moveSpeed = data.Get("camera.moveSpeed").AsFloat(10.f);
    pSettings->camera.nearPlane = data.Get("camera.nearPlane").AsFloat(0.5f);
    pSettings->camera.farPlane = data.Get("camera.farPlane").AsFloat(10000.f);
    pSettings->camera.fieldOfView = data.Get("camera.fieldOfView").AsFloat(vcLens30mm);
    pSettings->camera.lensIndex = data.Get("camera.lensId").AsInt(vcLS_30mm);
    pSettings->camera.invertX = data.Get("camera.invertX").AsBool(false);
    pSettings->camera.invertY = data.Get("camera.invertY").AsBool(false);
    pSettings->camera.moveMode = (vcCameraMoveMode)data.Get("camera.moveMode").AsInt(0);

    // Map Tiles
    pSettings->maptiles.mapEnabled = data.Get("maptiles.enabled").AsBool(true);
    pSettings->maptiles.blendMode = (vcMapTileBlendMode)data.Get("maptiles.blendMode").AsInt(1);
    pSettings->maptiles.transparency = data.Get("maptiles.transparency").AsFloat(1.f);
    pSettings->maptiles.mapHeight = data.Get("maptiles.mapHeight").AsFloat(0.f);

    // Docks
    vcSettings_LoadDocks(data);
  }

  if (pSavedData != pDefaultSettings)
    udFree(pSavedData);

  return true;
}

bool vcSettings_Save(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage)
    return false;

  bool success = false;

  if (pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  ImGui::GetIO().WantSaveIniSettings = false;

  udValue data;

  // Misc Settings
  if (pSettings->showFPS) // This hides the option if its false
    data.Set("showFPS = true");

  // Windows
  data.Set("window.position.x = %d", pSettings->window.xpos);
  data.Set("window.position.y = %d", pSettings->window.ypos);
  data.Set("window.width = %d", pSettings->window.width);
  data.Set("window.height = %d", pSettings->window.height);
  data.Set("window.maximized = %s", pSettings->window.maximized ? "true" : "false");
  data.Set("window.fullscreen = %s", pSettings->window.fullscreen ? "true" : "false");

  data.Set("frames.scene = %s", pSettings->window.windowsOpen[vcdScene] ? "true" : "false");
  data.Set("frames.settings = %s", pSettings->window.windowsOpen[vcdSettings] ? "true" : "false");
  data.Set("frames.explorer = %s", pSettings->window.windowsOpen[vcdSceneExplorer] ? "true" : "false");

  // Camera
  data.Set("camera.moveSpeed = %f", pSettings->camera.moveSpeed);
  data.Set("camera.nearPlane = %f", pSettings->camera.nearPlane);
  data.Set("camera.farPlane = %f", pSettings->camera.farPlane);
  data.Set("camera.fieldOfView = %f", pSettings->camera.fieldOfView);
  data.Set("camera.lensId = %i", pSettings->camera.lensIndex);
  data.Set("camera.invertX = %s", pSettings->camera.invertX ? "true" : "false");
  data.Set("camera.invertY = %s", pSettings->camera.invertY ? "true" : "false");
  data.Set("camera.moveMode = %d", pSettings->camera.moveMode);

  // Map Tiles
  data.Set("maptiles.enabled = %s", pSettings->maptiles.mapEnabled ? "true" : "false");
  data.Set("maptiles.blendMode = %d", pSettings->maptiles.blendMode);
  data.Set("maptiles.transparency = %f", pSettings->maptiles.transparency);
  data.Set("maptiles.mapHeight = %f", pSettings->maptiles.mapHeight);

  // Docks
  vcSettings_SaveDocks(data);

  const char *pSettingsStr;

  if (data.Export(&pSettingsStr) == udR_Success)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

    success = SDL_filewrite(buffer, pSettingsStr, udStrlen(pSettingsStr));

    udFree(pSettingsStr);
  }

  return success;
}
