#include "RuntimeDecorator.h"
#include <chrono>
#include <memory>
#include <unordered_map>
#include "LayoutAnimationsProxy.h"
#include "MutableValue.h"
#include "ReanimatedHiddenHeaders.h"

namespace reanimated {

std::unordered_map<RuntimePointer, RuntimeType>
    &RuntimeDecorator::runtimeRegistry() {
  static std::unordered_map<RuntimePointer, RuntimeType> runtimeRegistry;
  return runtimeRegistry;
}

void RuntimeDecorator::registerRuntime(
    jsi::Runtime *runtime,
    RuntimeType runtimeType) {
  runtimeRegistry().insert({runtime, runtimeType});
}

void RuntimeDecorator::decorateRuntime(
    jsi::Runtime &rt,
    const std::string &label) {
  // This property will be used to find out if a runtime is a custom worklet
  // runtime (e.g. UI, VisionCamera frame processor, ...)
  rt.global().setProperty(rt, "_WORKLET", jsi::Value(true));
  // This property will be used for debugging
  rt.global().setProperty(
      rt, "_LABEL", jsi::String::createFromAscii(rt, label));

  jsi::Object dummyGlobal(rt);
  rt.global().setProperty(rt, "global", dummyGlobal);

  rt.global().setProperty(rt, "jsThis", jsi::Value::undefined());

  auto callback = [](jsi::Runtime &rt,
                     const jsi::Value &thisValue,
                     const jsi::Value *args,
                     size_t count) -> jsi::Value {
    const jsi::Value *value = &args[0];
    if (value->isString()) {
      Logger::log(value->getString(rt).utf8(rt).c_str());
    } else if (value->isNumber()) {
      Logger::log(value->getNumber());
    } else if (value->isUndefined()) {
      Logger::log("undefined");
    } else {
      Logger::log("unsupported value type");
    }
    return jsi::Value::undefined();
  };
  jsi::Value log = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_log"), 1, callback);
  rt.global().setProperty(rt, "_log", log);

  auto setGlobalConsole = [](jsi::Runtime &rt,
                             const jsi::Value &thisValue,
                             const jsi::Value *args,
                             size_t count) -> jsi::Value {
    rt.global().setProperty(rt, "console", args[0]);
    return jsi::Value::undefined();
  };
  rt.global().setProperty(
      rt,
      "_setGlobalConsole",
      jsi::Function::createFromHostFunction(
          rt,
          jsi::PropNameID::forAscii(rt, "_setGlobalConsole"),
          1,
          setGlobalConsole));

  rt.global().setProperty(
      rt,
      "_chronoNow",
      jsi::Function::createFromHostFunction(
          rt,
          jsi::PropNameID::forAscii(rt, "_chronoNow"),
          0,
          [](jsi::Runtime &rt,
             const jsi::Value &thisValue,
             const jsi::Value *args,
             size_t count) -> jsi::Value {
            double now = std::chrono::system_clock::now().time_since_epoch() /
                std::chrono::milliseconds(1);
            return jsi::Value(now);
          }));
}

void RuntimeDecorator::decorateUIRuntime(
    jsi::Runtime &rt,
    const UpdatePropsFunction updateProps,
    const MeasureFunction measure,
#ifdef RCT_NEW_ARCH_ENABLED
    const RemoveShadowNodeFromRegistryFunction removeShadowNodeFromRegistry,
    const DispatchCommandFunction dispatchCommand,
#else
    const ScrollToFunction scrollTo,
#endif
    const RequestFrameFunction requestFrame,
    const TimeProviderFunction getCurrentTime,
    const RegisterSensorFunction registerSensor,
    const UnregisterSensorFunction unregisterSensor,
    const SetGestureStateFunction setGestureState,
    std::shared_ptr<LayoutAnimationsProxy> layoutAnimationsProxy) {
  RuntimeDecorator::decorateRuntime(rt, "UI");
  rt.global().setProperty(rt, "_UI", jsi::Value(true));

#ifdef RCT_NEW_ARCH_ENABLED
  auto clb = [updateProps](
                 jsi::Runtime &rt,
                 const jsi::Value &thisValue,
                 const jsi::Value *args,
                 const size_t count) -> jsi::Value {
    updateProps(rt, args[0], args[1]);
    return jsi::Value::undefined();
  };
  jsi::Value updatePropsHostFunction = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_updatePropsFabric"), 2, clb);
  rt.global().setProperty(rt, "_updatePropsFabric", updatePropsHostFunction);

  auto _removeShadowNodeFromRegistry = [removeShadowNodeFromRegistry](
                                           jsi::Runtime &rt,
                                           const jsi::Value &thisValue,
                                           const jsi::Value *args,
                                           const size_t count) -> jsi::Value {
    removeShadowNodeFromRegistry(rt, args[0]);
    return jsi::Value::undefined();
  };
  jsi::Value removeShadowNodeFromRegistryHostFunction =
      jsi::Function::createFromHostFunction(
          rt,
          jsi::PropNameID::forAscii(rt, "_removeShadowNodeFromRegistry"),
          2,
          _removeShadowNodeFromRegistry);
  rt.global().setProperty(
      rt,
      "_removeShadowNodeFromRegistry",
      removeShadowNodeFromRegistryHostFunction);

  auto clb3 = [dispatchCommand](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  const size_t count) -> jsi::Value {
    dispatchCommand(rt, args[0], args[1], args[2]);
    return jsi::Value::undefined();
  };
  jsi::Value dispatchCommandHostFunction =
      jsi::Function::createFromHostFunction(
          rt, jsi::PropNameID::forAscii(rt, "_dispatchCommand"), 3, clb3);
  rt.global().setProperty(rt, "_dispatchCommand", dispatchCommandHostFunction);

  auto _measure = [measure](
                      jsi::Runtime &rt,
                      const jsi::Value &thisValue,
                      const jsi::Value *args,
                      const size_t count) -> jsi::Value {
    return measure(rt, args[0]);
  };
#else
  auto clb = [updateProps](
                 jsi::Runtime &rt,
                 const jsi::Value &thisValue,
                 const jsi::Value *args,
                 const size_t count) -> jsi::Value {
    const auto viewTag = args[0].asNumber();
    const jsi::Value *viewName = &args[1];
    const auto params = args[2].asObject(rt);
    updateProps(rt, viewTag, *viewName, params);
    return jsi::Value::undefined();
  };
  jsi::Value updatePropsHostFunction = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_updatePropsPaper"), 3, clb);
  rt.global().setProperty(rt, "_updatePropsPaper", updatePropsHostFunction);

  auto _scrollTo = [scrollTo](
                       jsi::Runtime &rt,
                       const jsi::Value &thisValue,
                       const jsi::Value *args,
                       const size_t count) -> jsi::Value {
    int viewTag = static_cast<int>(args[0].asNumber());
    double x = args[1].asNumber();
    double y = args[2].asNumber();
    bool animated = args[3].getBool();
    scrollTo(viewTag, x, y, animated);
    return jsi::Value::undefined();
  };
  jsi::Value scrollToFunction = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_scrollTo"), 4, _scrollTo);
  rt.global().setProperty(rt, "_scrollTo", scrollToFunction);

  auto _measure = [measure](
                      jsi::Runtime &rt,
                      const jsi::Value &thisValue,
                      const jsi::Value *args,
                      const size_t count) -> jsi::Value {
    int viewTag = static_cast<int>(args[0].asNumber());
    auto result = measure(viewTag);
    jsi::Object resultObject(rt);
    for (auto &i : result) {
      resultObject.setProperty(rt, i.first.c_str(), i.second);
    }
    return resultObject;
  };
#endif // RCT_NEW_ARCH_ENABLED

  jsi::Value measureFunction = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_measure"), 1, _measure);
  rt.global().setProperty(rt, "_measure", measureFunction);

  auto clb2 = [requestFrame](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  const size_t count) -> jsi::Value {
    auto fun =
        std::make_shared<jsi::Function>(args[0].asObject(rt).asFunction(rt));
    requestFrame([&rt, fun](double timestampMs) {
      fun->call(rt, jsi::Value(timestampMs));
    });
    return jsi::Value::undefined();
  };
  jsi::Value requestAnimationFrame = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "requestAnimationFrame"), 1, clb2);
  rt.global().setProperty(rt, "requestAnimationFrame", requestAnimationFrame);

  auto clb6 = [getCurrentTime](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  const size_t count) -> jsi::Value {
    return getCurrentTime();
  };
  jsi::Value timeFun = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_getCurrentTime"), 0, clb6);
  rt.global().setProperty(rt, "_getCurrentTime", timeFun);

  rt.global().setProperty(rt, "_frameTimestamp", jsi::Value::undefined());
  rt.global().setProperty(rt, "_eventTimestamp", jsi::Value::undefined());

  // layout animation
  std::weak_ptr<LayoutAnimationsProxy> layoutProxy = layoutAnimationsProxy;
  auto clb7 = [layoutProxy](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  size_t count) -> jsi::Value {
    std::shared_ptr<LayoutAnimationsProxy> proxy = layoutProxy.lock();
    if (layoutProxy.expired()) {
      return jsi::Value::undefined();
    }
    proxy->startObserving(
        args[0].asNumber(),
        args[1].asObject(rt).getHostObject<MutableValue>(rt),
        rt);
    return jsi::Value::undefined();
  };
  jsi::Value _startObservingProgress = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_startObservingProgress"), 0, clb7);
  rt.global().setProperty(
      rt, "_startObservingProgress", _startObservingProgress);

  auto clb8 = [layoutProxy](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  size_t count) -> jsi::Value {
    std::shared_ptr<LayoutAnimationsProxy> proxy = layoutProxy.lock();
    if (layoutProxy.expired()) {
      return jsi::Value::undefined();
    }
    proxy->stopObserving(args[0].asNumber(), args[1].getBool());
    return jsi::Value::undefined();
  };
  jsi::Value _stopObservingProgress = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_stopObservingProgress"), 0, clb8);
  rt.global().setProperty(rt, "_stopObservingProgress", _stopObservingProgress);

  auto clb9 = [setGestureState](
                  jsi::Runtime &rt,
                  const jsi::Value &thisValue,
                  const jsi::Value *args,
                  size_t count) -> jsi::Value {
    int handlerTag = static_cast<int>(args[0].asNumber());
    int newState = static_cast<int>(args[1].asNumber());
    setGestureState(handlerTag, newState);
    return jsi::Value::undefined();
  };
  jsi::Value setGestureStateFunction = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "_setGestureState"), 2, clb9);
  rt.global().setProperty(rt, "_setGestureState", setGestureStateFunction);
}

} // namespace reanimated
