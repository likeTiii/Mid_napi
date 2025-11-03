import router from '@ohos.router';

import nativeModule from 'libentry.so'


@Entry
@Component
struct Visualization {
  @State message: string = 'Hello World';


  // XComponent实例对象的context,初始为空，等待onLoad被触发后填充
  // 键是字符串（函数名），值是 无参无返回的函数（() => void）
  private xComponentContext: Record<string, () => void> = {};



  build() {
    Column() {
      Row() {
        Text('Visualization here')
          .fontSize(25)
          .fontWeight(FontWeight.Bold)

        Button() {
          Text('Back')
            .fontSize(25)
            .fontWeight(FontWeight.Bold)
        }
          .type(ButtonType.Capsule)
          .backgroundColor('#0d9ffb')
          .onClick(()=>{
            router.back()
          })
        .margin(10)
      }
      .width('100%')
      .margin({top: 20})




      //XComponent
      XComponent({
        id: 'xComponentId',
        type: XComponentType.SURFACE,
        libraryname: 'entry'
      })
        //此处的xComponentContext是在初始化.so文件时自动组装注入的，由native侧提供，native侧export
        .onLoad((xComponentContext?: Object | Record<string, () => void>) => {
          if (xComponentContext) {
            this.xComponentContext = xComponentContext as Record<string, () => void>;
          }
        })

      Button('绘制图形')
        .onClick(() => {
          if (this.xComponentContext) {
            this.xComponentContext.drawRectangle();
          }
        })
    }
  }
}