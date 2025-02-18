// Code.gs

/**
 * スプレッドシートからRFIDリストを取得する関数
 */
function getRFIDList() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData');
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  const headers = data[0];
  const janCodeIndex = headers.findIndex(header => header === 'JANコード');
  
  // デバッグログ
  Logger.log('Headers:', headers);
  Logger.log('JAN Code Index:', janCodeIndex);
  
  // 1行目をヘッダーとして除外
  const rfidList = data.slice(1).map(row => {
    // デバッグログ
    Logger.log('Row data:', row);
    return {
      timestamp: row[0] instanceof Date ? row[0].toISOString() : String(row[0]),
      rfid: String(row[1]),
      maker: String(row[2]),
      category: String(row[3]),
      name: String(row[4]),
      janCode: janCodeIndex !== -1 ? String(row[5]) : '' // JANコードは6列目（インデックス5）にある
    };
  }).filter(item => item.rfid); // RFIDが空でないものをフィルタリング
  
  // デバッグログ
  Logger.log('RFID List:', JSON.stringify(rfidList));
  
  return rfidList;
}

/**
 * フォームから送信されたRFIDデータをスプレッドシートに登録または更新する関数
 * @param {Object} form - フォームデータ
 */
function processForm(form) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData');
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const rfidData = form.rfidData;
  const now = new Date();
  const timestamp = now.toISOString();

  // スプレッドシートの全RFIDを取得してマップを作成
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  const headers = data[0];
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
        // 新しいRFIDとして追加（空のJANコードフィールドを含む）
        newEntries.push([timestamp, rfid, '', '', '', '']); // タイムスタンプ, RFID, メーカー, カテゴリ, 名称, JANコード
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

/**
 * JANコードからRFIDを検索する関数
 * @param {string} janCode - 検索するJANコード
 * @returns {Array} - 見つかったRFIDのデータの配列
 */
function searchRFIDByJAN(janCode) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData');
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  
  // JANコードの列のインデックスを取得（ヘッダー行から）
  const headers = data[0];
  const janCodeIndex = headers.findIndex(header => header === 'JANコード');
  
  // デバッグログ
  Logger.log('Headers:', headers);
  Logger.log('JAN Code Index:', janCodeIndex);
  Logger.log('Searching for JAN code:', janCode);
  
  if (janCodeIndex === -1) {
    throw new Error('JANコードの列が見つかりません。');
  }
  
  // JANコードに一致する行を全て検索
  const results = [];
  for (let i = 1; i < data.length; i++) {
    const row = data[i];
    const currentJANCode = String(row[janCodeIndex]).trim();
    Logger.log(`Row ${i}: JAN code = ${currentJANCode}`);
    
    if (currentJANCode === String(janCode).trim()) {
      Logger.log('Match found in row:', i);
      const result = {
        timestamp: row[0] instanceof Date ? row[0].toISOString() : String(row[0]),
        rfid: String(row[1]),
        maker: String(row[2]),
        category: String(row[3]),
        name: String(row[4]),
        janCode: currentJANCode
      };
      Logger.log('Adding result:', result);
      results.push(result);
    }
  }
  
  Logger.log('Total results found:', results.length);
  Logger.log('Search results:', JSON.stringify(results));
  
  // 必ず配列を返す
  return results.length > 0 ? results : [];
}
