# AviUtl プラグイン - SplitWindow

AviUtl のウィンドウを再帰分割可能なペインに貼り付けられるようにします。
[最新バージョンをダウンロード](../../releases/latest/)

※ UniteWindow とは同時に使用できません。

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* SplitWindow.aul
	* SplitWindow.xml
	* SplitWindow (フォルダ)

## 使用方法

### ペインを分割する

1. ペインを右クリックしてメニューを表示します。
2. 「垂直分割」か「水平分割」を選択します。
3. (1)～(2) を繰り返します。

### ペインにウィンドウをドッキングする

1. ドッキングしたいウィンドウを表示状態にします。
2. ペインを右クリックしてメニューを表示します。
3. ドッキングしたいウィンドウを選択します。

### ドッキングを解除する

1. ペインのタイトルを右クリックしてメニューを表示します。
2. 「ウィンドウなし」を選択します。

### 分割を解除する

1. 分割を解除したいボーダーを右クリックしてメニューを表示します。
2. 「分割なし」を選択します。

## 設定方法

シングルウィンドウのタイトルバーを右クリックしてシステムメニューを表示します。

### レイアウトのインポート

レイアウトファイルを選択してインポートします。

### レイアウトのエクスポート

レイアウトファイルを選択してエクスポートします。

### SplitWindowの設定

SplitWindowの設定ダイアログを開きます。

* ```配色``` 背景色とボーダーの配色を指定します。
	* ```塗り潰し``` 背景色を指定します。
	* ```ボーダー``` ボーダーの色を指定します。
	* ```ホットボーダー``` ホットボーダーの色を指定します。

### 設定ファイル

SplitWindow.xml をテキストエディタで開いて編集します。

* ```<config>```
	* ```borderWidth``` ボーダーの幅を指定します。
	* ```captionHeight``` キャプションの高さを指定します。
	* ```borderSnapRange``` ボーダーのスナップ範囲を指定します。
	* ```fillColor``` 背景の塗りつぶし色を指定します。
	* ```borderColor``` ボーダーの色を指定します。
	* ```hotBorderColor``` ホット状態のボーダーの色を指定します。
	* ```<singleWindow>``` シングルウィンドウの位置を指定します。ウィンドウ位置がおかしくなった場合はこのタグを削除してください。
	* ```<pane>``` 入れ子にできます。
		* ```splitMode``` ```none```、```vert```、```horz``` のいずれかを指定します。
		* ```origin``` ```topLeft```、```bottomRight``` のいずれかを指定します。
		* ```border``` ```origin``` からの相対値でボーダーの座標を指定します。
		* ```window``` このペインにドッキングさせるウィンドウを指定します。
	* ```<floatWindow>``` フローティングウィンドウの位置を指定します。ウィンドウ位置がおかしくなった場合はこのタグを削除してください。

## 更新履歴

* 1.0.0 - 2022/06/24 初版

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r41 https://scrapbox.io/ePi5131/patch.aul

## クレジット

* Microsoft Research Detours Package https://github.com/microsoft/Detours
* aviutl_exedit_sdk https://github.com/ePi5131/aviutl_exedit_sdk
* Common Library https://github.com/hebiiro/Common-Library

## 作成者情報
 
* 作成者 - 蛇色 (へびいろ)
* GitHub - https://github.com/hebiiro
* Twitter - https://twitter.com/io_hebiiro

## 免責事項

このプラグインおよび同梱物を使用したことによって生じたすべての障害・損害・不具合等に関しては、私と私の関係者および私の所属するいかなる団体・組織とも、一切の責任を負いません。各自の責任においてご使用ください。
