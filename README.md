a protobuf compiler for golang

## Requirements
* Go 1.1 or higher        http://golang.org/
* Protobuf 2.4 or higher  http://code.google.com/p/protobuf/

## Build
```
cd src
make
make install
```

## Test
run these in shell
```
cd example
make
bin/main
```

## Benchmark
Serialize/Parse a simple message on
```
message Test {
    message Sub {
        required int32 f1 = 1;
        required int32 f2 = 2;
        optional int32 f3 = 3;
    }
    
    required int32 f1 = 1;
    required int64 f2 = 2;
    optional string f3 = 3;
    optional bytes f4 = 4;
    optional Sub f5 = 5;
    repeated int32 f6 = 6;
}
```
<table>
    <tr>
        <th>Benchmark</th>
        <th>gopbuf</th>
        <th>goprotobuf</th>
        <th>protobuf(cpp)</th>
        <th>protobuf(cpp +O3)</th>
    </tr>
    <tr>
        <th>Serialize(1000000)</th>
        <td>0.483s</td>
        <td>1.364s</td>
        <td>0.862s</td>
        <td>0.141s</td>
    </tr>
    <tr>
        <th>Parse(1000000)</th>
        <td>1.393s</td>
        <td>2.165s</td>
        <td>0.696s</td>
        <td>0.231s</td>
    </tr> 
    
</table>
