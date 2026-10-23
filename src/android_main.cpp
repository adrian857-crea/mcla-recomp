
#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_sdl.h>

// SDL3 Android loads libmain.so and invokes SDL_main().
extern "C" SDL_DECLSPEC int SDLCALL SDL_main(int argc, char* argv[]) {
  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();

  int result = EXIT_FAILURE;

  {
    rex::ui::SDLWindowedAppContext context;

    if (!context.Initialize()) {
      REXLOG_ERROR("Failed to initialize SDL3 UI context.");
      return EXIT_FAILURE;
    }

    auto creator = rex::ui::WindowedApp::GetCreator("mcla");

    if (!creator) {
      REXLOG_ERROR("MCLA app creator was not registered.");
      return EXIT_FAILURE;
    }

    std::unique_ptr<rex::ui::WindowedApp> app = creator(context);

    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;

    const size_t count =
        std::min(remaining.size(), option_names.size());

    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }

    app->SetParsedArguments(std::move(parsed));

    if (app->OnInitialize()) {
      result = context.RunMainMessageLoop();
    } else {
      REXLOG_ERROR("MCLA ReXApp initialization failed.");
    }

    app->InvokeOnDestroy();
  }

  return result;
}
