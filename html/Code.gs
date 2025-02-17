// Code.gs

/**
 * スプレッドシートからRFIDリストを取得する関数
 */
function getRFIDList() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData'); // シート名を適宜変更してください
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  
  // 1行目をヘッダーとして除外
  const rfidList = data.slice(1).map(row => ({
    timestamp: row[0] instanceof Date ? row[0].toISOString() : String(row[0]),
    rfid: String(row[1]),
    maker: String(row[2]),
    category: String(row[3]),
    name: String(row[4])
  })).filter(item => item.rfid); // RFIDが空でないものをフィルタリング
  
  Logger.log('RFID List: ' + JSON.stringify(rfidList)); // デバッグ用ログ
  
  return rfidList;
}

/**
 * フォームから送信されたRFIDデータをスプレッドシートに登録または更新する関数
 * @param {Object} form - フォームデータ
 */
function processForm(form) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData'); // シート名を適宜変更してください
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const rfidData = form.rfidData;
  const now = new Date();
  const timestamp = now.toISOString();

  // スプレッドシートの全RFIDを取得してマップを作成
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  const existingRFIDs = {};
  for (let i = 1; i < data.length; i++) { // 1行目はヘッダー
    const rfid = String(data[i][1]).trim();
    if (rfid) {
      existingRFIDs[rfid] = i + 1; // 行番号（1-based）
    }
  }

  // トランザクションを使用して一括処理
  const updates = [];
  const newEntries = [];

  rfidData.forEach(rfid => {
    rfid = String(rfid).trim();
    if (rfid) {
      if (existingRFIDs.hasOwnProperty(rfid)) {
        // 既存のRFIDのタイムスタンプを更新
        updates.push({
          row: existingRFIDs[rfid],
          timestamp: timestamp
        });
      } else {
        // 新しいRFIDとして追加
        newEntries.push([timestamp, rfid]);
      }
    }
  });

  // タイムスタンプの更新
  updates.forEach(update => {
    sheet.getRange(update.row, 1).setValue(update.timestamp);
  });

  // 新規RFIDの追加
  if (newEntries.length > 0) {
    sheet.getRange(sheet.getLastRow() + 1, 1, newEntries.length, newEntries[0].length).setValues(newEntries);
  }

  return '成功';
}

/**
 * 選択されたRFIDをスプレッドシートから削除する関数
 * @param {Array} selectedRFIDs - 削除するRFIDの配列
 */
function deleteRFIDs(selectedRFIDs) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData'); // シート名を適宜変更してください
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  
  // RFIDが選択されていない行を保持（ヘッダーを除外）
  const rowsToKeep = data.slice(1).filter(row => !selectedRFIDs.includes(row[1]));
  
  // シートをクリア
  sheet.clearContents();
  
  // ヘッダーを再度書き込む（1行目）
  const headers = data[0];
  sheet.appendRow(headers);
  
  // データが存在する場合のみ書き込み（ヘッダー以降）
  if (rowsToKeep.length > 0) {
    sheet.getRange(2, 1, rowsToKeep.length, rowsToKeep[0].length).setValues(rowsToKeep);
  }
}

/**
 * ウェブアプリとしての設定
 */
function doGet(e) {
  return HtmlService.createHtmlOutputFromFile('index') // デフォルトで登録フォームを表示
      .setTitle('RFID管理システム');
}
