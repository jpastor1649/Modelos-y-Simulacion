;; --- PROBLEMA #2 (NETLOGO) ---
;; ModSim_303
;; - Jaime Darley Angulo Tenorio
;; - John Alejandro Pastor Sandoval
;; - Juan Camilo Vergara Tao
;; - Juan Diego Velásquez Pinzón

;; Razas de agentes
breed [domiciliarios domiciliario]  ; Motociclistas
breed [residencias residencia]      ; Casas y apartamentos

;; Variables globales
globals [
  restaurante
  pedidos-pendientes      ; Lista de pedidos esperando ser asignados
  pedidos-entregados      ; Contador de pedidos ya entregados
  tiempo-total-espera     ; Suma de todos los tiempos de espera
  tiempo-espera-promedio  ; Métrica principal
]

;; Variables propias de las residencias
residencias-own [
  es-edificio?          ; true si es edificio, false si es casa
  num-apartamentos      ; Número de apartamentos (si es edificio)
]

;; Variables propias de los domiciliarios
domiciliarios-own [
  estado                ; "en-base", "en-camino-1", "en-camino-2", "regresando"
  pedido-1              ; El primer pedido que lleva
  pedido-2              ; El segundo pedido que lleva (para Escenario 2)
  velocidad             ; Celdas por minuto
]

;; --- SETUPs ---

to setup-a
  clear-all
  setup-mundo
  setup-residencias
  setup-motos
  reset-ticks
end

to setup-b
  clear-all
  setup-mundo
  setup-residencias
  setup-ciclas
  reset-ticks
end

to setup-mundo
  ;; Crear el restaurante en el centro
  ask patch 0 0 [set pcolor red]
  set restaurante patch 0 0

  ;; Inicializar variables globales
  set pedidos-pendientes []
  set pedidos-entregados 0
  set tiempo-total-espera 0
  set tiempo-espera-promedio 0
end

to setup-residencias
  ;; Crear 50 residencias en celdas aleatorias y vacías
  repeat 50 [
    let target-patch one-of patches with [not any? residencias-here]
    ask target-patch [
      sprout-residencias 1 [
        set shape "house"
        ;; Determinar si es casa o edificio
        ifelse random-float 1.0 < 0.7 [
          set es-edificio? true
          set num-apartamentos (4 + random 17) ; Aleatorio entre 4 y 20
          set color blue
          set size 1.25
        ]
        [
          set es-edificio? false
          set num-apartamentos 1
          set color green
          set size 1.15
        ]
      ]
    ]
  ]
end

to setup-motos
  ;; Crear 'n' domiciliarios en el restaurante
  create-domiciliarios n-domiciliarios [
    setxy 0 0
    set shape "person"
    set color white
    set estado "en-base"
    set pedido-1 nobody
    set pedido-2 nobody
    set velocidad 20 / 60 ; Velocidad de las motocicletas
  ]
end

to setup-ciclas
  ;; Crear 'n' domiciliarios en el restaurante
  create-domiciliarios n-domiciliarios [
    setxy 0 0
    set shape "person"
    set color white
    set estado "en-base"
    set pedido-1 nobody
    set pedido-2 nobody
    set velocidad 10 / 60 ; Velocidad de las bicicletas
  ]
end

;; --- GOs ---

to go-1
  ;; Detener la simulación después de 180 minutos
  if ticks >= 10800 [
    stop
  ]
  generar-pedidos
  asignar-pedido-simple
  mover-domiciliarios-simple
  actualizar-metricas
  tick ; Avanza un minuto
end

to go-2
  ;; Detener la simulación después de 180 minutos
  if ticks >= 10800 [
    stop
  ]
  generar-pedidos
  asignar-pedidos-dobles
  mover-domiciliarios-dobles
  actualizar-metricas
  tick ; Avanza un minuto
end

to generar-pedidos
  ;; Se generan 3 pedidos por minuto
  if ticks mod 60 = 0 [
    repeat 3 [
      let cliente one-of residencias
      ;; Un pedido es una lista: [agente-cliente, tiempo-de-creacion]
      let nuevo-pedido list cliente ticks
      set pedidos-pendientes lput nuevo-pedido pedidos-pendientes
    ]
  ]
end

to actualizar-metricas
  ;; Calcula el tiempo de espera promedio
  if pedidos-entregados > 0 [
    let promedio-en-segundos tiempo-total-espera / pedidos-entregados
    set tiempo-espera-promedio promedio-en-segundos / 60
  ]
end

to-report domiciliarios-activos
  report count domiciliarios - count domiciliarios with [estado = "en-base" or estado = "esperando-pedido"]
end

;; --- ESCENARIO 1: Pedidos simples ---

to asignar-pedido-simple
  ;; Asigna pedidos pendientes a domiciliarios en la base
  let domiciliarios-libres domiciliarios with [estado = "en-base"]

  while [any? domiciliarios-libres and length pedidos-pendientes > 0] [
    let repartidor one-of domiciliarios-libres
    let pedido-a-entregar first pedidos-pendientes

    ask repartidor [
      set pedido-1 pedido-a-entregar
      set estado "en-camino-1"
    ]

    ;; Quitar el pedido de la lista de pendientes
    set pedidos-pendientes remove-item 0 pedidos-pendientes
    ;; Actualizar la lista de domiciliarios libres
    set domiciliarios-libres domiciliarios with [estado = "en-base"]
  ]
end

to mover-domiciliarios-simple
  ask domiciliarios [
    ;; Si está en camino a entregar
    if estado = "en-camino-1" [
      let destino first pedido-1
      face destino
      ;; Si llega al destino
      ifelse distance destino < velocidad [
        move-to destino
        entregar-pedido-simple
      ]
      [
        fd velocidad
      ]
    ]

    ;; Si está regresando al restaurante
    if estado = "regresando" [
      face restaurante
      ;; Si llega al restaurante
      ifelse distance restaurante < velocidad [
        move-to restaurante
        set estado "en-base"
      ]
      [
        fd velocidad
      ]
    ]
  ]
end

to entregar-pedido-simple
  ;; Este procedimiento es llamado por el domiciliario que llega al destino
  let tiempo-creacion item 1 pedido-1
  let tiempo-espera (ticks - tiempo-creacion)

  set tiempo-total-espera tiempo-total-espera + tiempo-espera
  set pedidos-entregados pedidos-entregados + 1

  set pedido-1 nobody
  set estado "regresando"
end

;; --- ESCENARIO 2: Pedidos dobles ---

to asignar-pedidos-dobles
  ;; Busca domiciliarios que puedan tomar un pedido
  let domiciliarios-disponibles domiciliarios with [estado = "en-base" or estado = "esperando-pedido"]

  while [any? domiciliarios-disponibles and length pedidos-pendientes > 0] [
    let repartidor one-of domiciliarios-disponibles
    let pedido-a-entregar first pedidos-pendientes

    ask repartidor [
      ;; Si ya tiene un pedido, este es el segundo
      ifelse pedido-1 != nobody [
        set pedido-2 pedido-a-entregar
        set estado "en-camino-1" ; Sigue en camino al primero, pero ya tiene el segundo listo
      ]
      [ ; Si está esperando, este es su segundo pedido
        ifelse estado != "en-base" [
            set pedido-2 pedido-a-entregar
            set estado "en-camino-2"
          ]
          [ ; Si está en base, este es su primer pedido
            set pedido-1 pedido-a-entregar
            set estado "en-camino-1"
          ]
      ]
    ]

    set pedidos-pendientes remove-item 0 pedidos-pendientes
    set domiciliarios-disponibles domiciliarios with [estado = "en-base" or (estado = "esperando-pedido" and pedido-2 = nobody)]
  ]
end

to mover-domiciliarios-dobles
  ask domiciliarios [
    ifelse estado = "en-camino-1" [
      let destino first pedido-1
      face destino
      ifelse distance destino < velocidad [
        move-to destino
        entregar-pedido-1-doble
      ]
      [
        fd velocidad
      ]
    ]
    [
      ifelse estado = "en-camino-2" [
        let destino first pedido-2
        face destino
        ifelse distance destino < velocidad[
          move-to destino
          entregar-pedido-2-doble
        ]
        [
          fd velocidad
        ]
      ]
      [
        if estado = "regresando" [
          face restaurante
          ifelse distance restaurante < velocidad [
            move-to restaurante
            set estado "en-base"
          ]
          [
            fd velocidad
          ]
        ]
      ]
    ]
  ]
end

to entregar-pedido-1-doble
  ;; Procesa la entrega del primer pedido
  let tiempo-creacion item 1 pedido-1
  set tiempo-total-espera tiempo-total-espera + (ticks - tiempo-creacion)
  set pedidos-entregados pedidos-entregados + 1
  set pedido-1 nobody

  ;; Decide qué hacer ahora
  ifelse pedido-2 != nobody [
    set estado "en-camino-2" ; Hay un segundo pedido, ir hacia él
  ]
  [
    set estado "esperando-pedido" ; No hay segundo pedido, esperar aquí
  ]
end

to entregar-pedido-2-doble
  ;; Procesa la entrega del segundo pedido
  let tiempo-creacion item 1 pedido-2
  set tiempo-total-espera tiempo-total-espera + (ticks - tiempo-creacion)
  set pedidos-entregados pedidos-entregados + 1
  set pedido-2 nobody
  set estado "regresando" ; Después del segundo, siempre regresa
end
@#$#@#$#@
GRAPHICS-WINDOW
79
10
591
523
-1
-1
10.044444444444444
1
10
1
1
1
0
0
0
1
-22
22
-22
22
0
0
1
ticks
30.0

BUTTON
603
13
780
54
🏍 Setup (Motocicletas) 🏍
setup-a
NIL
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

BUTTON
606
229
785
284
 Simulación Pedidos Simples 📦
go-1
T
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

SLIDER
607
171
784
204
n-domiciliarios
n-domiciliarios
1
10
5.0
1
1
Domiciliarios
HORIZONTAL

MONITOR
608
387
789
436
Tiempo de Espera Promedio
tiempo-espera-promedio
17
1
12

MONITOR
731
455
789
504
Minutos
ticks / 60
0
1
12

PLOT
800
230
1266
358
Pedidos Entregados
Segundos
Pedidos
0.0
10800.0
0.0
10.0
true
false
"" ""
PENS
"pedidos-entregados" 1.0 0 -955883 true "" "plot pedidos-entregados"

PLOT
798
14
1265
205
Tiempo de Espera Promedio
Segundos
Minutos
0.0
10800.0
0.0
1.0
true
false
"" ""
PENS
"tiempo-espera-promedio" 1.0 0 -14070903 true "" "plot tiempo-espera-promedio"

BUTTON
606
302
786
357
Simulación Pedidos Dobles 📦 📦
go-2
T
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

BUTTON
605
89
783
130
🚲 Setup (Bicicletas) 🚲
setup-b
NIL
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

TEXTBOX
608
60
758
78
Velocidad de motocicletas: 20
11
0.0
1

TEXTBOX
608
138
758
156
Velocidad de bicicletas: 10
11
0.0
1

MONITOR
608
457
718
502
Pedidos Entregados
pedidos-entregados
0
1
11

PLOT
801
385
1267
505
Domiciliarios Activos
Segundos
Domiciliarios
0.0
10800.0
0.0
1.0
true
false
"" ""
PENS
"default" 1.0 0 -15040220 true "" "plot domiciliarios-activos"

@#$#@#$#@
## WHAT IS IT?

(a general understanding of what the model is trying to show or explain)

## HOW IT WORKS

(what rules the agents use to create the overall behavior of the model)

## HOW TO USE IT

(how to use the model, including a description of each of the items in the Interface tab)

## THINGS TO NOTICE

(suggested things for the user to notice while running the model)

## THINGS TO TRY

(suggested things for the user to try to do (move sliders, switches, etc.) with the model)

## EXTENDING THE MODEL

(suggested things to add or change in the Code tab to make the model more complicated, detailed, accurate, etc.)

## NETLOGO FEATURES

(interesting or unusual features of NetLogo that the model uses, particularly in the Code tab; or where workarounds were needed for missing features)

## RELATED MODELS

(models in the NetLogo Models Library and elsewhere which are of related interest)

## CREDITS AND REFERENCES

(a reference to the model's URL on the web if it has one, as well as any other necessary credits, citations, and links)
@#$#@#$#@
default
true
0
Polygon -7500403 true true 150 5 40 250 150 205 260 250

airplane
true
0
Polygon -7500403 true true 150 0 135 15 120 60 120 105 15 165 15 195 120 180 135 240 105 270 120 285 150 270 180 285 210 270 165 240 180 180 285 195 285 165 180 105 180 60 165 15

arrow
true
0
Polygon -7500403 true true 150 0 0 150 105 150 105 293 195 293 195 150 300 150

box
false
0
Polygon -7500403 true true 150 285 285 225 285 75 150 135
Polygon -7500403 true true 150 135 15 75 150 15 285 75
Polygon -7500403 true true 15 75 15 225 150 285 150 135
Line -16777216 false 150 285 150 135
Line -16777216 false 150 135 15 75
Line -16777216 false 150 135 285 75

bug
true
0
Circle -7500403 true true 96 182 108
Circle -7500403 true true 110 127 80
Circle -7500403 true true 110 75 80
Line -7500403 true 150 100 80 30
Line -7500403 true 150 100 220 30

butterfly
true
0
Polygon -7500403 true true 150 165 209 199 225 225 225 255 195 270 165 255 150 240
Polygon -7500403 true true 150 165 89 198 75 225 75 255 105 270 135 255 150 240
Polygon -7500403 true true 139 148 100 105 55 90 25 90 10 105 10 135 25 180 40 195 85 194 139 163
Polygon -7500403 true true 162 150 200 105 245 90 275 90 290 105 290 135 275 180 260 195 215 195 162 165
Polygon -16777216 true false 150 255 135 225 120 150 135 120 150 105 165 120 180 150 165 225
Circle -16777216 true false 135 90 30
Line -16777216 false 150 105 195 60
Line -16777216 false 150 105 105 60

car
false
0
Polygon -7500403 true true 300 180 279 164 261 144 240 135 226 132 213 106 203 84 185 63 159 50 135 50 75 60 0 150 0 165 0 225 300 225 300 180
Circle -16777216 true false 180 180 90
Circle -16777216 true false 30 180 90
Polygon -16777216 true false 162 80 132 78 134 135 209 135 194 105 189 96 180 89
Circle -7500403 true true 47 195 58
Circle -7500403 true true 195 195 58

circle
false
0
Circle -7500403 true true 0 0 300

circle 2
false
0
Circle -7500403 true true 0 0 300
Circle -16777216 true false 30 30 240

cow
false
0
Polygon -7500403 true true 200 193 197 249 179 249 177 196 166 187 140 189 93 191 78 179 72 211 49 209 48 181 37 149 25 120 25 89 45 72 103 84 179 75 198 76 252 64 272 81 293 103 285 121 255 121 242 118 224 167
Polygon -7500403 true true 73 210 86 251 62 249 48 208
Polygon -7500403 true true 25 114 16 195 9 204 23 213 25 200 39 123

cylinder
false
0
Circle -7500403 true true 0 0 300

dot
false
0
Circle -7500403 true true 90 90 120

face happy
false
0
Circle -7500403 true true 8 8 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Polygon -16777216 true false 150 255 90 239 62 213 47 191 67 179 90 203 109 218 150 225 192 218 210 203 227 181 251 194 236 217 212 240

face neutral
false
0
Circle -7500403 true true 8 7 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Rectangle -16777216 true false 60 195 240 225

face sad
false
0
Circle -7500403 true true 8 8 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Polygon -16777216 true false 150 168 90 184 62 210 47 232 67 244 90 220 109 205 150 198 192 205 210 220 227 242 251 229 236 206 212 183

fish
false
0
Polygon -1 true false 44 131 21 87 15 86 0 120 15 150 0 180 13 214 20 212 45 166
Polygon -1 true false 135 195 119 235 95 218 76 210 46 204 60 165
Polygon -1 true false 75 45 83 77 71 103 86 114 166 78 135 60
Polygon -7500403 true true 30 136 151 77 226 81 280 119 292 146 292 160 287 170 270 195 195 210 151 212 30 166
Circle -16777216 true false 215 106 30

flag
false
0
Rectangle -7500403 true true 60 15 75 300
Polygon -7500403 true true 90 150 270 90 90 30
Line -7500403 true 75 135 90 135
Line -7500403 true 75 45 90 45

flower
false
0
Polygon -10899396 true false 135 120 165 165 180 210 180 240 150 300 165 300 195 240 195 195 165 135
Circle -7500403 true true 85 132 38
Circle -7500403 true true 130 147 38
Circle -7500403 true true 192 85 38
Circle -7500403 true true 85 40 38
Circle -7500403 true true 177 40 38
Circle -7500403 true true 177 132 38
Circle -7500403 true true 70 85 38
Circle -7500403 true true 130 25 38
Circle -7500403 true true 96 51 108
Circle -16777216 true false 113 68 74
Polygon -10899396 true false 189 233 219 188 249 173 279 188 234 218
Polygon -10899396 true false 180 255 150 210 105 210 75 240 135 240

house
false
0
Rectangle -7500403 true true 45 120 255 285
Rectangle -16777216 true false 120 210 180 285
Polygon -7500403 true true 15 120 150 15 285 120
Line -16777216 false 30 120 270 120

leaf
false
0
Polygon -7500403 true true 150 210 135 195 120 210 60 210 30 195 60 180 60 165 15 135 30 120 15 105 40 104 45 90 60 90 90 105 105 120 120 120 105 60 120 60 135 30 150 15 165 30 180 60 195 60 180 120 195 120 210 105 240 90 255 90 263 104 285 105 270 120 285 135 240 165 240 180 270 195 240 210 180 210 165 195
Polygon -7500403 true true 135 195 135 240 120 255 105 255 105 285 135 285 165 240 165 195

line
true
0
Line -7500403 true 150 0 150 300

line half
true
0
Line -7500403 true 150 0 150 150

pentagon
false
0
Polygon -7500403 true true 150 15 15 120 60 285 240 285 285 120

person
false
0
Circle -7500403 true true 110 5 80
Polygon -7500403 true true 105 90 120 195 90 285 105 300 135 300 150 225 165 300 195 300 210 285 180 195 195 90
Rectangle -7500403 true true 127 79 172 94
Polygon -7500403 true true 195 90 240 150 225 180 165 105
Polygon -7500403 true true 105 90 60 150 75 180 135 105

plant
false
0
Rectangle -7500403 true true 135 90 165 300
Polygon -7500403 true true 135 255 90 210 45 195 75 255 135 285
Polygon -7500403 true true 165 255 210 210 255 195 225 255 165 285
Polygon -7500403 true true 135 180 90 135 45 120 75 180 135 210
Polygon -7500403 true true 165 180 165 210 225 180 255 120 210 135
Polygon -7500403 true true 135 105 90 60 45 45 75 105 135 135
Polygon -7500403 true true 165 105 165 135 225 105 255 45 210 60
Polygon -7500403 true true 135 90 120 45 150 15 180 45 165 90

sheep
false
15
Circle -1 true true 203 65 88
Circle -1 true true 70 65 162
Circle -1 true true 150 105 120
Polygon -7500403 true false 218 120 240 165 255 165 278 120
Circle -7500403 true false 214 72 67
Rectangle -1 true true 164 223 179 298
Polygon -1 true true 45 285 30 285 30 240 15 195 45 210
Circle -1 true true 3 83 150
Rectangle -1 true true 65 221 80 296
Polygon -1 true true 195 285 210 285 210 240 240 210 195 210
Polygon -7500403 true false 276 85 285 105 302 99 294 83
Polygon -7500403 true false 219 85 210 105 193 99 201 83

square
false
0
Rectangle -7500403 true true 30 30 270 270

square 2
false
0
Rectangle -7500403 true true 30 30 270 270
Rectangle -16777216 true false 60 60 240 240

star
false
0
Polygon -7500403 true true 151 1 185 108 298 108 207 175 242 282 151 216 59 282 94 175 3 108 116 108

target
false
0
Circle -7500403 true true 0 0 300
Circle -16777216 true false 30 30 240
Circle -7500403 true true 60 60 180
Circle -16777216 true false 90 90 120
Circle -7500403 true true 120 120 60

tree
false
0
Circle -7500403 true true 118 3 94
Rectangle -6459832 true false 120 195 180 300
Circle -7500403 true true 65 21 108
Circle -7500403 true true 116 41 127
Circle -7500403 true true 45 90 120
Circle -7500403 true true 104 74 152

triangle
false
0
Polygon -7500403 true true 150 30 15 255 285 255

triangle 2
false
0
Polygon -7500403 true true 150 30 15 255 285 255
Polygon -16777216 true false 151 99 225 223 75 224

truck
false
0
Rectangle -7500403 true true 4 45 195 187
Polygon -7500403 true true 296 193 296 150 259 134 244 104 208 104 207 194
Rectangle -1 true false 195 60 195 105
Polygon -16777216 true false 238 112 252 141 219 141 218 112
Circle -16777216 true false 234 174 42
Rectangle -7500403 true true 181 185 214 194
Circle -16777216 true false 144 174 42
Circle -16777216 true false 24 174 42
Circle -7500403 false true 24 174 42
Circle -7500403 false true 144 174 42
Circle -7500403 false true 234 174 42

turtle
true
0
Polygon -10899396 true false 215 204 240 233 246 254 228 266 215 252 193 210
Polygon -10899396 true false 195 90 225 75 245 75 260 89 269 108 261 124 240 105 225 105 210 105
Polygon -10899396 true false 105 90 75 75 55 75 40 89 31 108 39 124 60 105 75 105 90 105
Polygon -10899396 true false 132 85 134 64 107 51 108 17 150 2 192 18 192 52 169 65 172 87
Polygon -10899396 true false 85 204 60 233 54 254 72 266 85 252 107 210
Polygon -7500403 true true 119 75 179 75 209 101 224 135 220 225 175 261 128 261 81 224 74 135 88 99

wheel
false
0
Circle -7500403 true true 3 3 294
Circle -16777216 true false 30 30 240
Line -7500403 true 150 285 150 15
Line -7500403 true 15 150 285 150
Circle -7500403 true true 120 120 60
Line -7500403 true 216 40 79 269
Line -7500403 true 40 84 269 221
Line -7500403 true 40 216 269 79
Line -7500403 true 84 40 221 269

wolf
false
0
Polygon -16777216 true false 253 133 245 131 245 133
Polygon -7500403 true true 2 194 13 197 30 191 38 193 38 205 20 226 20 257 27 265 38 266 40 260 31 253 31 230 60 206 68 198 75 209 66 228 65 243 82 261 84 268 100 267 103 261 77 239 79 231 100 207 98 196 119 201 143 202 160 195 166 210 172 213 173 238 167 251 160 248 154 265 169 264 178 247 186 240 198 260 200 271 217 271 219 262 207 258 195 230 192 198 210 184 227 164 242 144 259 145 284 151 277 141 293 140 299 134 297 127 273 119 270 105
Polygon -7500403 true true -1 195 14 180 36 166 40 153 53 140 82 131 134 133 159 126 188 115 227 108 236 102 238 98 268 86 269 92 281 87 269 103 269 113

x
false
0
Polygon -7500403 true true 270 75 225 30 30 225 75 270
Polygon -7500403 true true 30 75 75 30 270 225 225 270
@#$#@#$#@
NetLogo 6.4.0
@#$#@#$#@
@#$#@#$#@
@#$#@#$#@
<experiments>
  <experiment name="motocicletas-pedidos-simples" repetitions="3" sequentialRunOrder="false" runMetricsEveryStep="true">
    <setup>setup-a</setup>
    <go>go-1</go>
    <metric>tiempo-espera-promedio</metric>
    <runMetricsCondition>ticks &gt;= 10800</runMetricsCondition>
    <steppedValueSet variable="n-domiciliarios" first="1" step="1" last="10"/>
  </experiment>
  <experiment name="motocicletas-pedidos-dobles" repetitions="3" sequentialRunOrder="false" runMetricsEveryStep="true">
    <setup>setup-a</setup>
    <go>go-2</go>
    <metric>tiempo-espera-promedio</metric>
    <runMetricsCondition>ticks &gt;= 10800</runMetricsCondition>
    <steppedValueSet variable="n-domiciliarios" first="1" step="1" last="10"/>
  </experiment>
  <experiment name="bicicletas-pedidos-simples" repetitions="3" sequentialRunOrder="false" runMetricsEveryStep="true">
    <setup>setup-b</setup>
    <go>go-1</go>
    <metric>tiempo-espera-promedio</metric>
    <runMetricsCondition>ticks &gt;= 10800</runMetricsCondition>
    <steppedValueSet variable="n-domiciliarios" first="1" step="1" last="10"/>
  </experiment>
  <experiment name="bicicletas-pedidos-dobles" repetitions="3" sequentialRunOrder="false" runMetricsEveryStep="true">
    <setup>setup-b</setup>
    <go>go-2</go>
    <metric>tiempo-espera-promedio</metric>
    <runMetricsCondition>ticks &gt;= 10800</runMetricsCondition>
    <steppedValueSet variable="n-domiciliarios" first="1" step="1" last="10"/>
  </experiment>
</experiments>
@#$#@#$#@
@#$#@#$#@
default
0.0
-0.2 0 0.0 1.0
0.0 1 1.0 0.0
0.2 0 0.0 1.0
link direction
true
0
Line -7500403 true 150 150 90 180
Line -7500403 true 150 150 210 180
@#$#@#$#@
0
@#$#@#$#@
