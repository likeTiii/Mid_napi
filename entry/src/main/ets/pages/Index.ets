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

  @State inputs: string[] = ["",""];
  @State results: string[] = ["",""];
  @State message2: string = 'Test Node-API nativeCallArkTS result: ';
  @State bagFileList: string[] = [];
  @State selectedBagFile: string = '';
  TAG: string = "MasterConfigCopy";
  filesDir: string = '';
  @State fileOutputContent: string = '';
  @State logs: string[] = [];

  runCommand(index:number) {
    let context = getContext(this) as common.UIAbilityContext;
    let fileDir = context.filesDir; // 应用沙箱 files 目录
    let recordPath = this.filesDir;
    try{
      try {
        fs.mkdirSync(recordPath);
      } catch (err) {
        // 如果目录已存在会抛异常，这里忽略
        console.info("recordPath already exists: " + recordPath);
      }
      let result = nativeModule.runCommand(this.inputs[index], this.filesDir, index);
      if (typeof result === 'string'){
        this.results[index] = result ; //显示内容
      }
      else {
        this.results[index] = "Running...";
      }
      //this.resultText = nativeModule.runCommand(input);
    } catch (e) {
      this.results[index] = "Error: " + e.message;
    }
  }

  stopCommand(index:number){
    try {
      let input=this.inputs[index];
      let result = nativeModule.stopCommand(input);
      if (typeof result === 'string') {
        this.results[index] = result;
      } else {
        this.results[index] = "Stopped.";
      }
    } catch (e) {
      this.results[index] = "Error: " + e.message;
    }
  }

  messageCallback() {
    nativeModule.registerMessageCallback((rawMsg: string) => {
      const sep = rawMsg.indexOf('|');
      if (sep > 0) {
        const idx = Number(rawMsg.substring(0, sep));
        const msg = rawMsg.substring(sep + 1);

        this.results[idx] = (this.results[idx] ? this.results[idx] + '\n' : '') + msg;
      } else {
        console.error("Invalid log format: " + rawMsg);
      }
    });
  }


  aboutToAppear() {
    this.messageCallback();
    //获取沙箱路径
    let context = getContext(this) as common.UIAbilityContext;
    this.filesDir = context.filesDir;
    //创建文件
    FileUtils.createFile(this.filesDir);
    //获取文件列表
    this.bagFileList = FileUtils.getFileList(this.filesDir);
  }

  build() {
    Row() { // ✅ 改为 Row，实现左右布局
      // 左侧：文件工具，20%
      Column({ space: 20 }) {
        Text('Run Command')
          .fontSize(20)
          .margin(10)

        ForEach(this.inputs, (value: string, index: number) => {
          Column({ space: 10 }) {
            Row() {
              TextInput({ placeholder: 'Enter Command: ', text: this.inputs[index] })
                .width('70%')
                .onChange((val: string) => {
                  this.inputs[index] = val;
                })

              Button("Run")
                .onClick(() => this.runCommand(index))
                .width('20%')

              Button("Stop")
                .onClick(() => this.stopCommand(index))
                .width('20%')
            }

            Scroll() {
              Text(this.results[index])
                .fontSize(16)
                .textAlign(TextAlign.Start)
                .width('100%')
            }
            .height('20%')
            .width('100%')
            .backgroundColor(Color.Gray)
          }
        }, (value: string, index: number) => index.toString())

        Row() {
          Select(
            this.bagFileList.map((v: string): SelectOption => ({ value: v }))
          )
            .onSelect((index: number) => {
              this.selectedBagFile = this.bagFileList[index];
            })
            .backgroundColor(Color.Grey)
            .width('70%')

          Button("获取文件列表")
            .width('10%')
            .onClick(() => {
              this.bagFileList = FileUtils.getFileList(this.filesDir);
            })

          Button("read file")
            .width('10%')
            .onClick(() => {
              this.fileOutputContent = FileUtils.readFile(this.filesDir, this.selectedBagFile);
            })
        }

        Scroll() {
          Text(this.fileOutputContent)
            .fontSize(16)
            .textAlign(TextAlign.Start)
            .width('100%')
        }
        .height('20%')
        .width('100%')
        .backgroundColor(Color.Gray)
      }
      .padding('5%')
      .width('20%')
      .height('100%') // ✅ 确保高度撑满

      // 右侧：可视化工具，80%
      Column() {
        Row() {
          TextInput({ placeholder: '输入X坐标' })
            .layoutWeight(1)
            .margin(4)
          TextInput({ placeholder: '输入Y坐标' })
            .layoutWeight(1)
            .margin(4)
          TextInput({ placeholder: '输入Z坐标' })
            .layoutWeight(1)
            .margin(4)
          Button('确定')
        }
        .width('100%')
        .height(40)

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
      .height('100%')
      .width('80%')
    }
    .width('100%')
    .height('100%')
  }
}







