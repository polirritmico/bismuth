// SPDX-FileCopyrightText: 2022 Mikhail Zolotukhin <mail@gikari.com>
// SPDX-License-Identifier: MIT

#include "engine.hpp"
#include "config.hpp"
#include "engine/surface.hpp"
#include "logger.hpp"
#include "plasma-api/api.hpp"

namespace Bismuth
{
Engine::Engine(PlasmaApi::Api &api, const Bismuth::Config &config)
    : m_config(config)
    , m_windows(api.workspace())
    , m_activeLayouts(config)
    , m_plasmaApi(api)
{
}

void Engine::addWindow(PlasmaApi::Client client)
{
    // Don't manage special windows - docks, panels, etc.
    if (client.specialWindow()) {
        return;
    }

    auto &newWindow = m_windows.add(client);

    auto surfaces = newWindow.surfaces();

    arrangeWindowsOnSurfaces(surfaces);

    qDebug(Bi) << "New Window appears on" << surfaces.size() << "surfaces!";

    // Bind events of this window
}

void Engine::removeWindow(PlasmaApi::Client client)
{
    m_windows.remove(client);
}

void Engine::arrangeWindowsOnAllSurfaces()
{
    auto allSurfaces = [this]() -> std::vector<Surface> {
        auto currentActivity = m_plasmaApi.workspace().currentActivity();

        auto result = std::vector<Surface>();

        for (auto desktop = 1; desktop <= m_plasmaApi.workspace().desktops(); desktop++) {
            for (auto screen = 0; screen < m_plasmaApi.workspace().numScreens(); screen++) {
                result.push_back(Surface(desktop, screen, currentActivity));
            }
        }

        return result;
    };

    arrangeWindowsOnSurfaces(allSurfaces());
}

void Engine::arrangeWindowsOnVisibleSurfaces()
{
    auto screenSurfaces = [this]() -> std::vector<Surface> {
        auto currentDesktop = m_plasmaApi.workspace().currentDesktop();
        auto currentActivity = m_plasmaApi.workspace().currentActivity();
        auto result = std::vector<Surface>(1, Surface(currentDesktop, 0, currentActivity));

        // Add from additional screens
        for (auto screen = 1; screen < m_plasmaApi.workspace().numScreens(); screen++) {
            result.push_back(Surface(currentDesktop, screen, currentActivity));
        }

        return result;
    };

    arrangeWindowsOnSurfaces(screenSurfaces());
}

void Engine::arrangeWindowsOnSurfaces(const std::vector<Surface> &surfaces)
{
    for (auto &surface : surfaces) {
        arrangeWindowsOnSurface(surface);
    }
}

void Engine::arrangeWindowsOnSurface(const Surface &surface)
{
    auto &layout = m_activeLayouts.layoutOnSurface(surface);
    auto tilingArea = layout.tilingArea(workingArea(surface));

    auto visibleWindows = m_windows.visibleWindowsOn(surface);
    auto windowsThatCanBeTiled = visibleWindows; // TODO: Filter windows

    layout.apply(tilingArea, windowsThatCanBeTiled);
}

QRect Engine::workingArea(const Surface &surface) const
{
    return m_plasmaApi.workspace().clientArea(PlasmaApi::Workspace::PlacementArea, surface.screen(), surface.desktop());
}

}
