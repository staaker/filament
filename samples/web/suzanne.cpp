/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>

#include <math/vec3.h>

#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include "filamesh.h"
#include "filaweb.h"

using namespace filament;
using namespace math;
using namespace std;
using namespace utils;

struct SuzanneApp {
    filaweb::Asset meshAsset;
    MeshHandle meshHandle;
    Material* mat;
    MaterialInstance* mi;
    Camera* cam;
    Entity sun;
};

static constexpr uint8_t MATERIAL_LIT_PACKAGE[] = {
    #include "generated/material/sandboxLit.inc"
};

static SuzanneApp app;

void setup(Engine* engine, View* view, Scene* scene) {

    app.mat = Material::Builder()
            .package((void*) MATERIAL_LIT_PACKAGE, sizeof(MATERIAL_LIT_PACKAGE))
            .build(*engine);

    app.mi = app.mat->createInstance();
    app.mi->setParameter("baseColor", float3{0.8f});
    app.mi->setParameter("metallic", 1.0f);
    app.mi->setParameter("roughness", 0.7f);
    app.mi->setParameter("clearCoat", 0.0f);

    app.meshAsset = filaweb::getRawFile("mesh");
    const uint8_t* mdata = app.meshAsset.data.get();
    const auto destructor = [](void* buffer, size_t size, void* user) {
        app.meshAsset.data.reset();
    };
    app.meshHandle = decodeMesh(*engine, mdata, 0, app.mi, destructor, &app);

    scene->addEntity(app.meshHandle->renderable);

    view->setClearColor({0.1, 0.125, 0.25, 1.0});

    auto& em = EntityManager::get();
    app.sun = em.create();
    LightManager::Builder(LightManager::Type::SUN)
            .color(Color::toLinear<ACCURATE>({ 0.98f, 0.92f, 0.89f }))
            .intensity(110000)
            .direction({ 0.7, -1, -0.8 })
            .sunAngularRadius(1.2f)
            .castShadows(true)
            .build(*engine, app.sun);
    scene->addEntity(app.sun);

    app.cam = engine->createCamera();
    app.cam->setExposure(16.0f, 1 / 125.0f, 100.0f);
    app.cam->lookAt(float3{0}, float3{0, 0, -4});
    view->setCamera(app.cam);
};

void animate(Engine* engine, View* view, double now) {

    const uint32_t width = view->getViewport().width;
    const uint32_t height = view->getViewport().height;
    double ratio = double(width) / height;
    app.cam->setProjection(45.0, ratio, 0.1, 50.0, Camera::Fov::VERTICAL);

    auto& tcm = engine->getTransformManager();
    tcm.setTransform(tcm.getInstance(app.meshHandle->renderable),
        mat4f{mat3f{1.0}, float3{0.0f, 0.0f, -4.0f}} *
        mat4f::rotate(now, math::float3{0, 1, 0}));
};

void gui(filament::Engine* engine, filament::View*) {
};

// This is called only after the JavaScript layer has created a WebGL 2.0 context and all assets
// have been downloaded.
extern "C" void launch() {

    auto albedo = filaweb::getTexture("albedo");
    albedo.data.reset();

    filaweb::Application::get()->run(setup, gui, animate);
}

// The main() entry point is implicitly called after JIT compilation, but potentially before the
// WebGL context has been created or assets have finished loading.
int main() { }

