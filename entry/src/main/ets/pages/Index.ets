import hilog from '@ohos.hilog';
import nativeModule from 'libentry.so';
import common from '@ohos.app.ability.common';
import fs from '@ohos.file.fs';
import { BusinessError } from '@ohos.base';
//文件相关接口
import * as FileUtils from '../utils/FileUtils'
import router from '@ohos.router';
import XComponentContext from "../interface/XComponentContext"





@Entry
@Component
struct Index {
  @State currentStatus: string = "init";
  private xComponentContext: XComponentContext | undefined = undefined;

  build() {
    Column() {


      Column({ space: 0 }) {
        XComponent({
          id: 'xcomponentId',
          type: XComponentType.SURFACE,
          libraryname: 'entry'
        })
          .onLoad((xComponentContext) => {
            this.xComponentContext = xComponentContext as XComponentContext;
            this.currentStatus = "index";
          })
          .onDestroy(() => {
            console.log('onDestroy');
          })
          .id("xcomponent")


      }

      .height('100%')
      .width('100%')


    }
    .width('100%')
    .height('100%')

  }
}
// struct Index {
//   @State inputs: string[] = ["",""];
//   @State results: string[] = ["",""];
//   @State message2: string = 'Test Node-API nativeCallArkTS result: ';
//   @State bagFileList: string[] = [];
//   @State selectedBagFile: string = '';
//   TAG: string = "MasterConfigCopy";
//   filesDir: string = '';
//   @State fileOutputContent: string = '';
//
//   runCommand(index:number) {
//     let context = getContext(this) as common.UIAbilityContext;
//     let fileDir = context.filesDir; // 应用沙箱 files 目录
//     let recordPath = fileDir + "/bag_records";
//
//     try{
//       try {
//         fs.mkdirSync(recordPath);
//       } catch (err) {
//         // 如果目录已存在会抛异常，这里忽略
//         console.info("recordPath already exists: " + recordPath);
//       }
//       let input=this.inputs[index];
//       let result = nativeModule.runCommand(input,recordPath);
//       if (typeof result === 'string'){
//         this.results[index] = result ; //显示内容
//       }
//       else {
//         this.results[index] = "Running...";
//       }
//       //this.resultText = nativeModule.runCommand(input);
//     } catch (e) {
//       this.results[index] = "Error: " + e.message;
//     }
//   }
//
//
//   aboutToAppear() {
//     //获取沙箱路径
//     let context = getContext(this) as common.UIAbilityContext;
//     this.filesDir = context.filesDir;
//     //创建文件
//     FileUtils.createFile(this.filesDir);
//     //获取文件列表
//     this.bagFileList = FileUtils.getFileList(this.filesDir);
//   }
//
//   build(){
//     Column(
//       { space: 20}){
//       Text('Run Command')
//         .fontSize(20)
//         .margin(10)
//       // Row(){
//       //   TextInput({ placeholder: 'Enter Command: ', text: this.inputText})
//       //     .width('70%')
//       //     .onChange((value: string) => {
//       //       this.inputText = value;
//       //     })
//       //
//       //   Button("Run Command")
//       //     .onClick(() => this.runCommand(this.inputText))
//       //     .width('20%')
//       // }
//       // 循环渲染多个输入框和按钮
//       ForEach(this.inputs, (value: string, index: number) => {
//         Column({ space: 10 }) {
//           Row() {
//             TextInput({ placeholder: 'Enter Command: ', text: this.inputs[index] })
//               .width('70%')
//               .onChange((val: string) => {
//                 this.inputs[index] = val;
//               })
//
//             Button("Run")
//               .onClick(() => this.runCommand(index))
//               .width('20%')
//           }
//
//           Scroll() {
//             Text(this.results[index])
//               .fontSize(16)
//               .textAlign(TextAlign.Start)
//               .width('100%')
//           }
//           .height('20%')
//           .width('100%')
//           .backgroundColor(Color.Gray)
//         }
//       }, (value: string, index: number) => index.toString()) // key 生成
//
//       Row(){
//         Select(
//           this.bagFileList                      // 原始数组
//             .map((v: string)                    // 遍历数组，v 是当前文件名
//               : SelectOption=> ({ value: v })   // 声明返回值类型， "SelectOption”把字符串 v 变成对象 { value: v }
//             )
//         )
//           .onSelect((index: number) => {
//             this.selectedBagFile = this.bagFileList[index];
//
//           })
//           .backgroundColor(Color.Grey)
//           .width('70%')
//
//         Button("获取文件列表")
//           .width('10%')
//           .onClick(()=>{
//             this.bagFileList = FileUtils.getFileList(this.filesDir);
//           })
//         Button("read file")
//           .width('10%')
//           .onClick(()=>{
//               this.fileOutputContent = FileUtils.readFile(this.filesDir, this.selectedBagFile);
//           })
//       }
//
//
//       Scroll() {
//         Text(this.fileOutputContent)
//           .fontSize(16)
//           .textAlign(TextAlign.Start)
//           .width('100%')
//       }
//       .height('20%')
//       .width('100%')
//       .backgroundColor(Color.Gray)
//
//       Button() {
//         Text('visualization')
//           .fontSize(25)
//           .fontWeight(FontWeight.Bold)
//       }
//       .type(ButtonType.Capsule)
//       .margin({top: 20})
//       .backgroundColor('#0d9ffb')
//       .width('40')
//       .height('5%')
//       .onClick(()=>{
//         router.pushUrl({url: 'pages/visualization'})
//       })
//
//     }.padding('5%')
//   }
//
// }


