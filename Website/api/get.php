<?php
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

const NORM_PM10 = 50;
const NORM_PM2_5 = 25;
const NORM_PM1_0 = 15;

function GetDataFullResolution($rows, $percentage = False) {
    $cnt = count($rows);
    for ($i = 0; $i < $cnt; $i++) {
        $row = $rows[$i];
        
        $date = DateTime::createFromFormat('Y-m-d H:i:s', $row['date']);
        $date->setTime ( $date->format("H"), $date->format("i"), 0 );
        
        $labels[] = $date->format("Y-m-d H:i");
        if ($percentage) {
            $pm10[] = round(100 * ($row['pm10'] / NORM_PM10), 2);
            $pm2_5[] = round(100 * ($row['pm2_5'] / NORM_PM2_5), 2);;
            $pm1_0[] = round(100 * ($row['pm1_0'] / NORM_PM1_0), 2);;
        } else {
            $pm10[] = $row['pm10'];
            $pm2_5[] = $row['pm2_5'];
            $pm1_0[] = $row['pm1_0'];
        }
        $humidity[] = $row['humidity'];
        $temperature[] = $row['temperature'];
        $pressure[] = round($row['pressure'] / 100, 2);
    }

    $ret = array (
        "labels"  => $labels,
        "pm10" => $pm10,
        "pm2_5" => $pm2_5,
        "pm1" => $pm1_0,
        'humidity' => $humidity,
        'temperature' => $temperature,
        'pressure' => $pressure
    );

    return $ret;
}

function GetDataHourly($rows, $percentage = False) {
    $cnt = count($rows);
    $i = 0;
    while ($i < $cnt) {
        $row = $rows[$i];
        $date = DateTime::createFromFormat('Y-m-d H:i:s', $row['date']);
        $date->setTime ( $date->format("H"), 0, 0 );
        $hour = $date->format("H");
        $_pm10 = 0;
        $_pm2_5 = 0;
        $_pm1_0 = 0;
        $_temp = 0;
        $_humid = 0;
        $_press = 0;
        $_count = 0;

        while(true) {
            $_count++;
            if ($percentage) {
                $_pm10 += round(100 * ($row['pm10'] / NORM_PM10), 2);
                $_pm2_5 += round(100 * ($row['pm2_5'] / NORM_PM2_5), 2);;
                $_pm1_0 += round(100 * ($row['pm1_0'] / NORM_PM1_0), 2);;
            } else {
                $_pm10 += $row['pm10'];
                $_pm2_5 += $row['pm2_5'];
                $_pm1_0 += $row['pm1_0'];
            }
            
            $_humid += $row['humidity'];
            $_temp += $row['temperature'];
            $_press += round($row['pressure'] / 100, 2);
            $i++;

            if ($i >= $cnt) break;

            $row = $rows[$i];
            $date_new = DateTime::createFromFormat('Y-m-d H:i:s', $row['date']);
            if ($date_new->format("H") != $hour) break;
        }
        
        $labels[] = $date->format("Y-m-d H:i");
        $pm10[] = round($_pm10 / $_count, 2);
        $pm2_5[] = round($_pm2_5 / $_count, 2);
        $pm1_0[] = round($_pm1_0 / $_count, 2);
        $humidity[] = round($_humid / $_count, 2);
        $temperature[] = round($_temp / $_count, 2);
        $pressure[] = round($_press / $_count, 2);
    }

    $ret = array (
        "labels"  => $labels,
        "pm10" => $pm10,
        "pm2_5" => $pm2_5,
        "pm1" => $pm1_0,
        'humidity' => $humidity,
        'temperature' => $temperature,
        'pressure' => $pressure
    );

    return $ret;
}

function GetDataDaily($rows, $percentage = False) {
    $cnt = count($rows);
    $i = 0;
    while ($i < $cnt) {
        $row = $rows[$i];
        $date = DateTime::createFromFormat('Y-m-d H:i:s', $row['date']);
        $date->setTime ( 0, 0, 0 );
        $day = $date->format("d");
        $_pm10 = 0;
        $_pm2_5 = 0;
        $_pm1_0 = 0;
        $_temp = 0;
        $_humid = 0;
        $_press = 0;
        $_count = 0;

        while(true) {
            $_count++;
            if ($percentage) {
                $_pm10 += round(100 * ($row['pm10'] / NORM_PM10), 2);
                $_pm2_5 += round(100 * ($row['pm2_5'] / NORM_PM2_5), 2);;
                $_pm1_0 += round(100 * ($row['pm1_0'] / NORM_PM1_0), 2);;
            } else {
                $_pm10 += $row['pm10'];
                $_pm2_5 += $row['pm2_5'];
                $_pm1_0 += $row['pm1_0'];
            }
            
            $_humid += $row['humidity'];
            $_temp += $row['temperature'];
            $_press += round($row['pressure'] / 100, 2);
            $i++;

            if ($i >= $cnt) break;

            $row = $rows[$i];
            $date_new = DateTime::createFromFormat('Y-m-d H:i:s', $row['date']);
            if ($date_new->format("d") != $day) break;
        }
        
        $labels[] = $date->format("Y-m-d H:i");
        $pm10[] = round($_pm10 / $_count, 2);
        $pm2_5[] = round($_pm2_5 / $_count, 2);
        $pm1_0[] = round($_pm1_0 / $_count, 2);
        $humidity[] = round($_humid / $_count, 2);
        $temperature[] = round($_temp / $_count, 2);
        $pressure[] = round($_press / $_count, 2);
    }

    $ret = array (
        "labels"  => $labels,
        "pm10" => $pm10,
        "pm2_5" => $pm2_5,
        "pm1" => $pm1_0,
        'humidity' => $humidity,
        'temperature' => $temperature,
        'pressure' => $pressure
    );

    return $ret;
}

include_once "config.php";

$pmPercent = False;
$averaging = "hour";
$startDate = DateTime::createFromFormat('Y-m-d', date("Y-m-d", time() - (7*24*60*60)));
$endDate = DateTime::createFromFormat('Y-m-d', date("Y-m-d", time()));

if (isset($_GET['pmPercent']) && $_GET['pmPercent'] != "") $pmPercent = $_GET['pmPercent'] === 'true'? true : false;
if (isset($_GET['avg']) && $_GET['avg'] != "") $averaging = $_GET['avg'];
if (isset($_GET['startDate']) && $_GET['startDate'] != "") $startDate = DateTime::createFromFormat('Y-m-d', $_GET['startDate']);
if (isset($_GET['endDate']) && $_GET['endDate'] != "") $endDate = DateTime::createFromFormat('Y-m-d', $_GET['endDate']);

$startDate->setTime ( 0, 0, 0 );
$endDate->setTime ( 0, 0, 0 );
$maxDays = 31;
if ($averaging === "day") $maxDays = 366;

$dateDiff = $startDate->diff($endDate, true);
if (intval($dateDiff->format('%a')) > $maxDays) {
    $startDate = clone $endDate;
    $startDate->sub(new DateInterval('P31D'));
}

$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT * FROM `dane` WHERE date > '" . $startDate->format('Y-m-d H:i:s') . "' AND date < '" . $endDate->format('Y-m-d H:i:s') . "';";

if ($data = $conn->query($sql)) {
    $rows = $data->fetch_all(MYSQLI_ASSOC);
    if ($averaging === "none") $toSerialize = GetDataFullResolution($rows, $pmPercent);
    else if ($averaging === "hour") $toSerialize = GetDataHourly($rows, $pmPercent);
    else $toSerialize = GetDataDaily($rows, $pmPercent);

    //header('Access-Control-Allow-Origin: *'); 
    header('Content-type: application/json');
    echo json_encode($toSerialize);

    $data->close();
}

$conn->close();

?>