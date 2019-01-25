<?php
include_once "config.php";

$json = json_decode(file_get_contents("php://input"), true);

if ($json['secret_key'] == $secret_key) {
    $temperature = $json['temperature'];
    $humidity = $json['humidity'];
    $pressure = $json['pressure'];
    $pm1_0 = $json['pm1_0'];
    $pm2_5 = $json['pm2_5'];
    $pm10 = $json['pm10'];
    $temperature_ts = $json['temperature_ts'];
    $humidity_ts = $json['humidity_ts'];
    $pressure_ts = $json['pressure_ts'];
    $pm_ts = $json['pm_ts'];

    $conn = new mysqli($servername, $username, $password, $dbname);

    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }

    $sql = "INSERT INTO `dane` (`id`, `date`, `temperature`, `humidity`, `pressure`, `pm1_0`, `pm2_5`, `pm10`, `temperature_ts`, `humidity_ts`, `pressure_ts`, `pm_ts`) 
            VALUES (NULL, CURRENT_TIMESTAMP, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    $stmt = $conn->prepare($sql);
    $stmt->bind_param("dddiiissss", $temperature, $humidity, $pressure, $pm1_0, $pm2_5, $pm10,
                date("Y-m-d H:i:s", $temperature_ts), date("Y-m-d H:i:s", $humidity_ts), date("Y-m-d H:i:s", $pressure_ts), date("Y-m-d H:i:s", $pm_ts));

    if ($stmt->execute()) {
        echo "OK";
    } else {
        echo "ERROR: " . $conn->error;
    }

    $conn->close();
} else {
    echo "ERROR: Wrong key: " . $json['secret_key'];
}
?>