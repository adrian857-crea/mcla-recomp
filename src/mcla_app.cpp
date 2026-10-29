
#include "generated/default/mcla_init.h"
#include "mcla_app.h"

#include <filesystem>
#include <string>

#if defined(__ANDROID__)
#include <SDL3/SDL.h>
#include <android/log.h>
#endif

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/filesystem/vfs.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>

REXCVAR_DEFINE_STRING(
    mcla_gpu_backend, "any", "MCLA",
    "Graphics backend for the Xenos GPU plugin: any, vulkan or d3d12");

namespace {

void MclaAppBootLog(const std::string& message) {
#if defined(__ANDROID__)
  __android_log_write(ANDROID_LOG_INFO, "MCLA_BOOT", message.c_str());
#else
  (void)message;
#endif
}

}  // namespace

void MclaApp::OnConfigurePaths(rex::PathConfig& paths) {
  MclaAppBootLog("PATH 01: OnConfigurePaths entrou");

#if defined(__ANDROID__)
  char* app_data = SDL_GetPrefPath("CentralGamer", "MCLA");

  if (app_data && *app_data) {
    MclaAppBootLog(
        std::string("PATH 02: armazenamento privado: ") + app_data);

    const std::filesystem::path data_path(app_data);
    paths.user_data_root = data_path;
    paths.cache_root = data_path / "cache";
  } else {
    MclaAppBootLog(
        std::string("PATH ERROR: SDL_GetPrefPath: ") + SDL_GetError());
  }

  SDL_free(app_data);
#endif

  if (!paths.game_data_root.empty()) {
    MclaAppBootLog(
        std::string("PATH 03: game_data_root ja definido: ") +
        paths.game_data_root.string());
    return;
  }

  std::filesystem::path folder =
      rex::filesystem::GetExecutableFolder();

  MclaAppBootLog(
      std::string("PATH 04: pasta inicial: ") + folder.string());

  for (int level = 0; level < 5 && !folder.empty(); ++level) {
    std::error_code error;
    const auto candidate = folder / "game" / "default.xex";

    MclaAppBootLog(
        std::string("PATH 05: verificando: ") + candidate.string());

    if (std::filesystem::exists(candidate, error)) {
      paths.game_data_root = folder / "game";

      MclaAppBootLog(
          std::string("PATH 06: XEX encontrado: ") +
          paths.game_data_root.string());
      return;
    }

    if (error) {
      MclaAppBootLog(
          std::string("PATH WARNING: erro ao verificar XEX: ") +
          error.message());
    }

    if (folder == folder.parent_path()) {
      break;
    }

    folder = folder.parent_path();
  }

  MclaAppBootLog(
      "PATH ERROR: busca automatica nao encontrou game/default.xex");
}

void MclaApp::OnPreSetup(rex::RuntimeConfig& config) {
  MclaAppBootLog("GPU 01: OnPreSetup entrou");

  config.audio_factory = &CreateMclaAudioSystem;

  if (config.gpu_plugin.empty()) {
    config.gpu_plugin = "xenos";
  }

  MclaAppBootLog(
      std::string("GPU 02: plugin solicitado: ") +
      config.gpu_plugin);

  const std::string backend = REXCVAR_GET(mcla_gpu_backend);

  MclaAppBootLog(
      std::string("GPU 03: backend: ") + backend);

  if (backend != "any") {
    config.graphics =
        rex::system::LoadGpuPlugin("xenos", backend);

    if (!config.graphics) {
      MclaAppBootLog(
          "GPU ERROR: falha ao carregar plugin com backend especifico");

      REXLOG_ERROR(
          "could not load the xenos GPU plugin with backend '{}'",
          backend);
    } else {
      MclaAppBootLog(
          "GPU 04: plugin carregado com backend especifico");
    }
  }
}

void MclaApp::OnPostSetup() {
  MclaAppBootLog("RUNTIME 01: OnPostSetup entrou");

  ConfigureFrameTiming();

  if (runtime() && runtime()->file_system()) {
    runtime()->file_system()->RegisterSymbolicLink(
        "t:", "\\Device\\Harddisk0\\Partition1");
  }

  MclaAppBootLog("RUNTIME 02: OnPostSetup terminou");
}
