window.chartColors = {
	red: 'rgb(255, 99, 132)',
	orange: 'rgb(255, 159, 64)',
	yellow: 'rgb(255, 205, 86)',
	green: 'rgb(75, 192, 192)',
	blue: 'rgb(54, 162, 235)',
	purple: 'rgb(153, 102, 255)',
	grey: 'rgb(201, 203, 207)'
};

var pmChart = null;
var pressChart = null;

function GetPmChart() {
    var pmApi = "http://powietrze.piotrsperka.info/api/get.php";
    var data = $('form').serialize();

    if ($("#dataSelect input[name='pmPercent']:checked").val() === 'false') {
        var pmLabel = "Pyły [μg/m³]";
    } else {
        var pmLabel = "Pyły [%]";
    }

    $.getJSON( pmApi, data).done(function( data ) {
        if (pmChart != null) pmChart.destroy();
        pmChart = new Chart(document.getElementById("pmChart").getContext('2d'), {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [{
                    label: 'PM10',
                    data: data.pm10,
                    borderColor: window.chartColors.green,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
					lineTension: 0
                }, {
                    label: 'PM2.5',
                    data: data.pm2_5,
                    borderColor: window.chartColors.blue,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
					lineTension: 0
                }, {
                    label: 'PM1.0',
                    data: data.pm1,
                    borderColor: window.chartColors.orange,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
					lineTension: 0
                }]
            },
            options: {
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero:true
                        },
                        scaleLabel: {
                            display: true,
                            labelString: pmLabel
                        }
                    }]
                }, 
                title: {
                    display: true,
                    text: 'Zapylenie'
                }
            }
        });

        if (pressChart != null) pressChart.destroy();
        pressChart = new Chart(document.getElementById("weatherChart").getContext('2d'), {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [{
                    label: 'Ciśnienie',
                    data: data.pressure,
                    borderColor: window.chartColors.green,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
                    lineTension: 0,
                    yAxisID: 'y-axis-1'
                }, {
                    label: 'Wilgotność',
                    data: data.humidity,
                    borderColor: window.chartColors.orange,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
                    lineTension: 0,
                    yAxisID: 'y-axis-2'
                }, {
                    label: 'Temperatura',
                    data: data.temperature,
                    borderColor: window.chartColors.blue,
					backgroundColor: 'rgba(0, 0, 0, 0)',
                    fill: false,
                    lineTension: 0,
                    yAxisID: 'y-axis-3'
                }]
            },
            options: {
                scales: {
                    yAxes: [{
                        type: 'linear', // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
                        display: true,
                        position: 'left',
                        id: 'y-axis-1',
                        scaleLabel: {
                            display: true,
                            labelString: 'Ciśnienie [hPa]'
                        }
                    }, {
                        type: 'linear', // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
                        display: true,
                        position: 'right',
                        id: 'y-axis-2',
                        scaleLabel: {
                            display: true,
                            labelString: 'Wilgotność [%]'
                        }
                    }, {
                        type: 'linear', // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
                        display: true,
                        position: 'right',
                        id: 'y-axis-3',
                        scaleLabel: {
                            display: true,
                            labelString: 'Temperatura [°C]'
                        }
                    }],
                }, 
                title: {
                    display: true,
                    text: 'Pogoda'
                }
            }
        });

      });

    
}

function OnIndexLoaded() {
    var startDate = new Date();
    startDate.setDate(startDate.getDate() - 7);
    var endDate = new Date();
    endDate.setDate(endDate.getDate() + 1);
    document.getElementById("endDate").valueAsDate = endDate;
    document.getElementById("startDate").valueAsDate = startDate;

    GetPmChart();
}