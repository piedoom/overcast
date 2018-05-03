module Main exposing (..)

import Html exposing (Html, text, div, h1, img, button)
import Html.Events exposing (onClick)
import Http
import Json.Decode as Decode
import LineChart.Dots as Dots
import LineChart
import Color
import Time exposing (Time, second)

---- HTTP ----
getData : Cmd Msg
getData =
    let
        url = 
            "http://192.168.1.18/"
        request = 
            Http.get url decodeData
    in
        Http.send NewSensorData request

decodeData : Decode.Decoder SensorData
decodeData =
    Decode.map5 SensorData
        (Decode.field "status" Decode.int)
        (Decode.field "unit" Decode.string)
        (Decode.field "temperature" Decode.float)
        (Decode.field "humidity" Decode.float)
        (Decode.field "time" Decode.float)

---- MODEL ----

type alias SensorData = 
    { status : Int
    , unit : String
    , temperature : Float
    , humidity : Float
    , time : Float
    }


type alias Model =
    { history : List(SensorData) }


init : ( Model, Cmd Msg )
init =
    ( 
        { history = [] 
        }, Cmd.none
    )



---- UPDATE ----


type Msg
    = NoOp
    | NewSensorData (Result Http.Error SensorData)
    | RequestSensorData Time


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        -- no operation
        NoOp ->
            (model, Cmd.none)

        -- getting new data
        NewSensorData(Ok result) ->
            ( { model | history = model.history ++ [result] }, Cmd.none)
                    
        NewSensorData(Err _) ->
            ( model, Cmd.none )

        -- start a request for new data
        RequestSensorData time ->
            ( model, getData )
        


---- SUBS ----
subs : Model -> Sub Msg
subs model = 
    Time.every 2000 RequestSensorData


---- VIEW ----


view : Model -> Html Msg
view model =
    div []
        [ chart model ]

chart : Model -> Html Msg
chart model =
    div []
    [ LineChart.view .time .temperature
        [ LineChart.line Color.red Dots.diamond "Temperature" model.history ]
    , LineChart.view .time .humidity 
        [ LineChart.line Color.blue Dots.circle "Humidity" model.history]
    ]
        

---- PROGRAM ----


main : Program Never Model Msg
main =
    Html.program
        { view = view
        , init = init
        , update = update
        , subscriptions = subs
        }
