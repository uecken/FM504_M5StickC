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
  
  // デバッグログ
  Logger.log('Headers:', headers);
  
  // 1行目をヘッダーとして除外
  const rfidList = data.slice(1).map(row => {
    // デバッグログ
    Logger.log('Row data:', row);
    return {
      timestamp: row[0] instanceof Date ? row[0].toISOString() : String(row[0]),
      productInfo: String(row[1]),
      janCode: String(row[2]),
      rfid: String(row[3]),
      maker: String(row[4]),
      category: String(row[5]),
      name: String(row[6])
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
  try {
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData');
    if (!sheet) {
      throw new Error('RFIDData シートが見つかりません。');
    }

    const now = new Date();
    const timestamp = Utilities.formatDate(now, 'Asia/Tokyo', 'yyyy-MM-dd HH:mm:ss');

    // デバッグログ
    Logger.log('Received form data:', JSON.stringify(form));
    Logger.log('RFID Data:', form.rfidData);

    // 新規エントリの作成
    const newEntries = [];
    form.rfidData.forEach(entry => {
      // 各フィールドを個別に取得して検証
      const productInfo = entry.productInfo || '';
      const janCode = entry.janCode;
      const rfid = entry.rfid;
      const maker = entry.maker;
      const category = entry.category;
      const name = entry.name;

      // デバッグログ
      Logger.log('Processing entry:', {
        timestamp: timestamp,
        productInfo: productInfo,
        janCode: janCode,
        rfid: rfid,
        maker: maker,
        category: category,
        name: name
      });

      // 新しいRFIDとして追加
      newEntries.push([
        timestamp,    // Timestamp
        productInfo,  // 商品情報
        janCode,      // JANcode
        rfid,         // RFID_EPC
        maker,        // Vendor
        category,     // Category
        name          // Name
      ]);
    });

    // デバッグログ
    Logger.log('Entries to be added:', newEntries);

    // 新規RFIDの追加
    if (newEntries.length > 0) {
      const lastRow = sheet.getLastRow();
      sheet.getRange(lastRow + 1, 1, newEntries.length, 7).setValues(newEntries);
      Logger.log('Successfully added entries at row:', lastRow + 1);
    }

    return '成功';
  } catch (error) {
    Logger.log('Error in processForm:', error.toString());
    throw new Error('データの処理中にエラーが発生しました: ' + error.message);
  }
}

/**
 * 選択されたRFIDをスプレッドシートから削除する関数
 * @param {Array} selectedRFIDs - 削除するRFIDの配列
 */
function deleteRFIDs(selectedRFIDs) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('RFIDData');
  if (!sheet) {
    throw new Error('RFIDData シートが見つかりません。');
  }
  
  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  
  // RFIDが選択されていない行を保持（ヘッダーを除外）
  // RFID_EPCは4列目（インデックス3）にある
  const rowsToKeep = data.slice(1).filter(row => !selectedRFIDs.includes(row[3]));
  
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
  const janCodeIndex = 2; // JANコードは3列目（インデックス2）
  
  // デバッグログ
  Logger.log('Headers:', headers);
  Logger.log('JAN Code Index:', janCodeIndex);
  Logger.log('Searching for JAN code:', janCode);
  
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
        productInfo: String(row[1]),
        janCode: currentJANCode,
        rfid: String(row[3]),
        maker: String(row[4]),
        category: String(row[5]),
        name: String(row[6])
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

/**
 * スプレッドシートの全シート名を取得する関数
 * @returns {Array} シート名の配列
 */
function getAllSheetNames() {
  const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
  const sheets = spreadsheet.getSheets();
  return sheets.map(sheet => sheet.getName());
}

/**
 * 指定されたシートからJANコードをインポートする関数
 * @param {string} sheetName - シート名
 * @returns {Array} JANコードの配列
 */
function importJANCodesFromSheet(sheetName) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(sheetName);
  if (!sheet) {
    throw new Error(`シート "${sheetName}" が見つかりません。`);
  }

  const dataRange = sheet.getDataRange();
  const data = dataRange.getValues();
  const headers = data[0];
  
  // JANコードの列インデックスを修正（3列目、インデックス2）
  const janCodeIndex = 2;
  
  // デバッグログを追加
  Logger.log('Sheet data:', data);
  Logger.log('Headers:', headers);
  Logger.log('JAN code index:', janCodeIndex);
  
  // ヘッダー行を除外し、JANコード列の値を取得
  const janCodes = data.slice(1)
    .map(row => String(row[janCodeIndex]).trim())
    .filter(code => code !== ''); // 空の値を除外
  
  // デバッグログを追加
  Logger.log('Extracted JAN codes:', janCodes);
  
  // 重複を除去して返す
  return [...new Set(janCodes)];
} 