
#include "generated/default/mcla_init.h"
#include "mcla_app.h"

#include <filesystem>

#if defined(__ANDROID__)
#include <SDL3/SDL.h>
#endif

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/filesystem/vfs.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>

REXCVAR_DEFINE_STRING(mcla_gpu_backend, "any", "MCLA",
                      "Graphics backend for the Xenos GPU plugin: any, vulkan or d3d12");

std::unique_ptr<rex::system::IAudioSystem> CreateMclaAudioSystem(
    rex::runtime::FunctionDispatcher* function_dispatcher);

void MclaApp::OnConfigurePaths(rex::PathConfig& paths) {
#if defined(__ANDROID__)
  // Use storage assigned to this app by SDL on Android.
  char* app_data = SDL_GetPrefPath("CentralGamer", "MCLA");
  if (app_data && *app_data) {
    const std::filesystem::path data_path(app_data);
    paths.user_data_root = data_path;
    paths.cache_root = data_path / "cache";
  }
  SDL_free(app_data);
#endif

  if (!paths.game_data_root.empty()) {
    return;
  }

  std::filesystem::path folder = rex::filesystem::GetExecutableFolder();
  for (int level = 0; level < 5 && !folder.empty(); ++level) {
    std::error_code error;
    if (std::filesystem::exists(folder / "game" / "default.xex", error)) {
      paths.game_data_root = folder / "game";
      return;
    }
    if (folder == folder.parent_path()) {
      break;
    }
    folder = folder.parent_path();
  }
}

void MclaApp::OnPreSetup(rex::RuntimeConfig& config) {
  config.audio_factory = &CreateMclaAudioSystem;
  if (config.gpu_plugin.empty()) {
    config.gpu_plugin = "xenos";
  }

  const std::string backend = REXCVAR_GET(mcla_gpu_backend);
  if (backend != "any") {
    config.graphics = rex::system::LoadGpuPlugin("xenos", backend);
    if (!config.graphics) {
      REXLOG_ERROR("could not load the xenos GPU plugin with backend '{}'",
                   backend);
    }
  }
}

void MclaApp::OnPostSetup() {
  ConfigureFrameTiming();

  if (runtime() && runtime()->file_system()) {
    runtime()->file_system()->RegisterSymbolicLink(
        "t:", "\\Device\\Harddisk0\\Partition1");
  }
}
