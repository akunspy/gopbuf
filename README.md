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

<table>
    <tr>
        <th>Benchmark</th>
        <th>gopbuf</th>
        <th>goprotobuf</th>
        <th>protobuf(cpp)</th>
        <th>protobuf(cpp +O3)</th>
    </tr>
    <tr>
        <th>Serialize</th>
        <td>0.483s</td>
        <td>1.364s</td>
        <td>0.862s</td>
        <td>0.141s</td>
    </tr>
    <tr>
        <th>Parse</th>
        <td>1.393s</td>
        <td>2.165s</td>
        <td>0.696s</td>
        <td>0.231s</td>
    </tr> 
    
</table>
