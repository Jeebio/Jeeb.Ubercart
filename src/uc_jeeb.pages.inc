<?php

require_once("jeeb_lib.php");

/**
 * Process Invoice Status Notifiations from Jeeb.
 */
function uc_jeeb_notification()
{
  $postdata = file_get_contents("php://input");
  $json = json_decode($postdata, true);

  jeeb_log("Entered Jeeb-Notification");
  if ( $json['stateId']== 2 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);$order = uc_order_load($json["orderNo"]);
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "StateId changed to 2.";
    uc_order_comment_save($order->order_id, 0, t('Invoice has been created.'), 'order', $payment_reason);
    $order->order_status = 'pending';
    uc_order_save($order);
  }
  else if ( $json['stateId']== 3 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);
    uc_cart_empty(uc_cart_get_id());
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "StateId changed to 3.";
    uc_order_comment_save($order->order_id, 0, t('Payment was received by Jeeb.'), 'order', $payment_reason);
    uc_order_comment_save($order->order_id, 0, t('Awaiting bitcoin confirmation'), 'order', $payment_reason);
    $order->order_status = 'payment_received';
    uc_order_save($order);
  }
  else if ( $json['stateId']== 4 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);
    $order = uc_order_load($json["orderNo"]);
    // $token = db_query("SELECT token FROM {uc_payment_jeeb} WHERE order_id = :order_id", array(':order_id' => $order->order_id))->fetchField();
    $token = $json["token"];
    $data = array(
      "token" => $token,
    );

    $data_string = json_encode($data);
    $api_key = variable_get('uc_jeeb_current_api_key');;
    $baseUri = "https://core.jeeb.io/api/" ;
    $url = $baseUri.'payments/' . $api_key . '/confirm';
    jeeb_log("Signature:".$api_key." Base-Url:".$baseUri." Url:".$url);

    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
    curl_setopt($ch, CURLOPT_POSTFIELDS, $data_string);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array(
        'Content-Type: application/json',
        'Content-Length: ' . strlen($data_string))
    );

    $result = curl_exec($ch);
    $data = json_decode( $result , true);
    jeeb_log("data = ".var_export($data, TRUE));


    if($data['result']['isConfirmed']){
      jeeb_log('Payment confirmed by jeeb');
      $comment = t('Secure Payment ID: @id confirmed by Jeeb.', array('@id' => $json["referenceNo"]));
      uc_payment_enter($order->order_id, 'jeeb', uc_order_get_total($order), $order->uid, NULL, $comment);
      uc_cart_complete_sale($order);
      $payment_reason = "Payment confirmed by jeeb";
      uc_order_comment_save($order->order_id, 0, t('Payment of @amount was confirmed by Jeeb.', array('@amount' => $json["value"])), 'order', $payment_reason);

      $order->order_status = 'completed';
      uc_order_save($order);

    }
    else {
      jeeb_log('Payment rejected by jeeb');
    }
  }
  else if ( $json['stateId']== 5 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "StateId changed to 5.";
    uc_order_comment_save($order->order_id, 0, t('Invoice was expired'), 'order', $payment_reason);
    $order->order_status = 'canceled';
    uc_order_save($order);
  }
  else if ( $json['stateId']== 6 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "StateId changed to 6.";
    uc_order_comment_save($order->order_id, 0, t('Over payment occurred'), 'order', $payment_reason);
    $order->order_status = 'canceled';
    uc_order_save($order);
  }
  else if ( $json['stateId']== 7 ) {
    jeeb_log('Order Id received = '.$json['orderNo'].' stateId = '.$json['stateId']);
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "StateId changed to 7.";
    uc_order_comment_save($order->order_id, 0, t('Under payment occurred'), 'order', $payment_reason);
    $order->order_status = 'canceled';
    uc_order_save($order);
  }
  else{
    jeeb_log('Cannot read state id sent by Jeeb');
    $order = uc_order_load($json["orderNo"]);
    $payment_reason = "Cannot read StateId";
    uc_order_comment_save($order->order_id, 0, t('Cannot read StateId. Contact Jeeb immediately.'), 'order', $payment_reason);
    $order->order_status = 'canceled';
    uc_order_save($order);
  }

} // end function - uc_jeeb_notification

// Handle the callback from the Jeeb server.
function uc_jeeb_callback(){
  jeeb_log("Entered callback with Order No. : ".$_POST["orderNo"]);
  jeeb_log("Data :".var_export($_POST, TRUE));
  $order = uc_order_load($_POST["orderNo"]);
  if($_POST["stateId"]==3){
    uc_cart_empty(uc_cart_get_id());
    drupal_set_message(t('Thank you for your order! Your order is now awaiting confirmation from Bitcoin Network.'));
    drupal_goto('cart');
  }
  else if($_POST["stateId"]==5){
    drupal_set_message(t('Your Jeeb Invoice was expired. Please feel free to continue shopping or contact us for assistance.'));
    drupal_goto('cart');
  }
  else if($_POST["stateId"]==6){
    drupal_set_message(t('Your Jeeb Invoice was over paid and was rejected by Jeeb. Please feel free to continue shopping or contact us for assistance.'));
    drupal_goto('cart');
  }
  else if($_POST["stateId"]==7){
    drupal_set_message(t('Your Jeeb Invoice was under paid and was rejected by Jeeb. Please feel free to continue shopping or contact us for assistance.'));
    drupal_goto('cart');
  }
} // end function uc_jeeb_callback
