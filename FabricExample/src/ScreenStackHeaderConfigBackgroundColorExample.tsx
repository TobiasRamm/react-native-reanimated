import React from 'react';
import {
  ScreenStack,
  Screen,
  ScreenStackHeaderConfig,
} from 'react-native-screens';
import Animated, {
  useAnimatedProps,
  useAnimatedStyle,
  useSharedValue,
} from 'react-native-reanimated';
import { StyleSheet, View } from 'react-native';
import {
  Gesture,
  GestureEvent,
  GestureDetector,
  GestureHandlerRootView,
} from 'react-native-gesture-handler';

declare const performance: {
  now: () => number;
};

const AnimatedScreenStackHeaderConfig = Animated.createAnimatedComponent(
  ScreenStackHeaderConfig
);

export default function EverythingExample() {
  const isPressed = useSharedValue(false);
  const offset = useSharedValue({ x: 0, y: 0 });

  const gesture = Gesture.Pan()
    .onBegin(() => {
      'worklet';
      isPressed.value = true;
    })
    .onChange((e: GestureEvent) => {
      'worklet';
      offset.value = {
        x: e.changeX + offset.value.x,
        y: e.changeY + offset.value.y,
      };
    })
    .onFinalize(() => {
      'worklet';
      isPressed.value = false;
    });

  const animatedStyles = useAnimatedStyle(() => {
    return {
      transform: [
        { translateX: offset.value.x },
        { translateY: offset.value.y },
      ],
      backgroundColor: isPressed.value ? 'gray' : 'black',
    };
  });

  const animatedProps = useAnimatedProps(() => {
    const h = Math.max(0, Math.min(360, Math.round(180 + offset.value.x)));
    const l = Math.max(0, Math.min(100, Math.round(50 - offset.value.y / 5)));
    const color = `hsl(${h}, 100%, ${l}%)`;
    return {
      backgroundColor: color,
      title: color,
    };
  }, [offset]);

  return (
    <GestureHandlerRootView style={styles.root}>
      <ScreenStack>
        <Screen>
          <AnimatedScreenStackHeaderConfig animatedProps={animatedProps} />
        </Screen>
      </ScreenStack>
      <View style={styles.container}>
        <GestureDetector gesture={gesture}>
          <Animated.View style={[styles.ball, animatedStyles]} />
        </GestureDetector>
      </View>
    </GestureHandlerRootView>
  );
}

const styles = StyleSheet.create({
  root: {
    flex: 1,
  },
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  ball: {
    width: 60,
    height: 60,
    borderRadius: 30,
    backgroundColor: 'black',
  },
});
