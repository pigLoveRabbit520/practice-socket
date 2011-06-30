create database mydatabase;

GRANT ALL PRIVILEGES ON mydatabase.* TO 'admin'@'%'
     IDENTIFIED BY 'admin' WITH GRANT OPTION;

use mydatabase;

create table userinfo(id int(4)  auto_increment primary key,
    username   varchar(24) unique not null,
    userpasswd varchar(24) not null,
    state      int(1)      not null);


insert into userinfo values(1000,'ftpadmin','ftpadmin');