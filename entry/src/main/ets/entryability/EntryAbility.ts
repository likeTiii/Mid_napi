import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import hilog from '@ohos.hilog';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
  }

  onDestroy(): void {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
  }

  // onWindowStageCreate(windowStage: window.WindowStage): void {
  //   // Main window is created, set main page for this ability
  //   hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');
  //
  //   windowStage.loadContent('pages/Index', (err) => {
  //     if (err.code) {
  //       hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
  //       return;
  //     }
  //     hilog.info(0x0000, 'testTag', 'Succeeded in loading the content.');
  //   });
  // }
  onWindowStageCreate(windowStage: window.WindowStage) {
    // Main window is created, set main page for this ability
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

    // 设置应用窗口全屏显示，允许内容绘制到状态栏和导航栏下方
    windowStage.getMainWindow((err, win) => {
      if (err.code) {
        hilog.error(0x0000, 'testTag', 'Failed to obtain the main window. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
      hilog.info(0x0000, 'testTag', 'Succeeded in obtaining the main window.');

      // 关键步骤 1: 设置窗口布局为全屏，允许UI扩展到安全区域之外
      win.setWindowLayoutFullScreen(true).then(() => {
        hilog.info(0x0000, 'testTag', 'Succeeded in setting the window layout to full-screen.');
      }).catch((reason) => {
        hilog.error(0x0000, 'testTag', 'Failed to set the window layout to full-screen. Cause: %{public}s', JSON.stringify(reason) ?? '');
      });

      // 隐藏状态栏和导航栏
      // 'status' 表示状态栏, 'navigation' 表示导航栏
      // // 传入一个空数组 [] 表示将它们都隐藏
      // win.setWindowSystemBarEnable([]).then(() => {
      //   hilog.info(0x0000, 'testTag', 'Succeeded in hiding the system bars.');
      // }).catch((reason) => {
      //   hilog.error(0x0000, 'testTag', 'Failed to hide the system bars. Cause: %{public}s', JSON.stringify(reason) ?? '');
      // });
    });

    // 加载主页面
    windowStage.loadContent('pages/Index', (err, data) => {
      if (err.code) {
        hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
      hilog.info(0x0000, 'testTag', 'Succeeded in loading the content. Data: %{public}s', JSON.stringify(data) ?? '');
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground(): void {
    // Ability has brought to foreground
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
  }
};
